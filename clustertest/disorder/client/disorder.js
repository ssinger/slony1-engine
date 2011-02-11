//
// The disorder application is a simple order system that is geared
// towards replication in that all read only transactions do consistency
// checks on the objects they select.
//
//


// ----
// Class disorder
//
// ----
function disorder (workerName) {
	// ----
	// DB connection and scaling factor (the $SCALING will be substituted
	// during here-doc processing of the Unix shell).
	// ----
	this.name	= workerName;
	this.dbcon	= null;

	this.debug_level = 0;

	this.debug = function (level, msg) {
		if (this.debug_level >= level)
			out.println(this.name + " [DEBUG] " + msg);
	}

	// ----
	// disorder.connect()
	//
	//	Will be called by the clustertest framework inside a worker
	//	thread to establish a database connection. No error handling
	//	is required. If this method throws an exception (for example
	//	cannot connect to database), the framework will call it again
	//	after the configured reconnect timeout.
	// ----
	this.connect = function (uri,user,pass) {
		if (this.dbcon) {
			this.dbcon.close();
			this.dbcon = null;
		}
		this.dbcon = java.sql.DriverManager.getConnection(uri, user, pass);

		this.debug(1, "connected");

		this.stmtGetConfig = null;
		this.stmtAdjustCountItem = null;
		this.stmtOrderGetCustomer = null;
		this.stmtShipGetOrders = null;
		this.stmtStatusLastOrderOfCustomer = null;
		this.stmtRestock = null;
		this.stmtUpdateGetCustomerSpecial = null;
		this.stmtItemCreate = null;
		this.stmtItemDelete = null;

		this.dbcon.setAutoCommit(false);
		var stmt = this.dbcon.createStatement();
		stmt.execute("set search_path to disorder, public");
		this.dbcon.commit();

		this.cfg_scaling = this.getConfig("scaling");
		this.cfg_nCustomer = this.getConfig("n_customer");
		this.cfg_aCustomer = this.getConfig("a_customer");
		this.cfg_nItem = this.getConfig("n_item");
		this.cfg_aItem = this.getConfig("a_item");
		this.cfg_nSpecial = this.getConfig("n_special");
		this.dbcon.rollback();

		this.trans_adjustCounts();
	}


	// ----
	// disorder.disconnect()
	//
	//	Called by the framework when it is time to disconnect. This can
	//	be due to an error condition before calling connect() again, or
	//	at the end of the test run.
	// ----
	this.disconnect = function () {
		this.dbcon.close();
		this.dbcon = null;

		this.debug(1, "disconnected");
	}


	// ----
	// getConfig()
	//
	// Function to get one configuration parameter saved in do_config
	// ----
	this.getConfig = function (opt) {
		if (!this.stmtGetConfig) {
			this.stmtGetConfig = this.dbcon.prepareStatement(
				"SELECT cfg_val FROM do_config " +
				"WHERE cfg_opt = ?");
		}

		this.stmtGetConfig.setString(1, opt);
		var res = this.stmtGetConfig.executeQuery();
		if (!res.next()) {
			this.disconnect();
			throw(new Error("config parameter " + opt + " not found"));
		}
		return res.getString(1);
	}


	// ----
	// disorder.trans_adjustCounts()
	//
	// An infrequent transaction that makes customers aware of new items.
	// ----
	this.trans_adjustCounts = function () {
		if (!this.stmtAdjustCountItem) {
			this.stmtAdjustCountItem = this.dbcon.prepareStatement(
				"SELECT count(*) FROM do_item "+
				" WHERE i_in_production");
			this.stmtAdjustMaxItem = this.dbcon.prepareStatement(
				"SELECT max(i_id) FROM do_item");
			this.stmtAdjustAvgTotalSold = this.dbcon.prepareStatement(
				"SELECT avg(ii_total_sold) FROM do_inventory");
		}

		var res = this.stmtAdjustCountItem.executeQuery();
		res.next();
		this.cfg_numItem = res.getInt(1);

		res = this.stmtAdjustMaxItem.executeQuery();
		res.next();
		this.cfg_maxItem = res.getInt(1);

		res = this.stmtAdjustAvgTotalSold.executeQuery();
		res.next();
		this.cfg_avgTotalSold = res.getInt(1);

		this.dbcon.rollback();
	}


	// ----
	// disorder.trans_orderCreate()
	//
	//	This method implements the disorder business transaction that
	//	creates new orders.
	// ----
	this.trans_orderCreate = function () {
		if (!this.stmtOrderGetCustomer) {
			this.stmtOrderGetCustomer = this.dbcon.prepareStatement(
				"SELECT c_name, c_total_orders, c_total_value "+
				" FROM do_customer "+
				" WHERE c_id = ? "+
				" FOR UPDATE OF do_customer");
			this.stmtOrderUpdateCustomer = this.dbcon.prepareStatement(
				"UPDATE do_customer "+
				" SET c_total_orders = c_total_orders + ?, "+
				"     c_total_value  = c_total_value + ? "+
				" WHERE c_id = ?");

			this.stmtOrderGetItem = this.dbcon.prepareStatement(
				"SELECT i_name, i_price, ii_in_stock - ii_reserved as ii_available"+
				" FROM do_item "+
				" JOIN do_inventory ON ii_id = i_id "+
				" WHERE i_id = ? "+
				" FOR UPDATE OF do_inventory");
			this.stmtOrderCreate = this.dbcon.prepareStatement(
				"INSERT INTO do_order "+
				" (o_c_id, o_order_lines, o_total_items, o_total_value, o_shipped) "+
				" values (?, ?, ?, ?, false)");
			this.stmtOrderCreateLine = this.dbcon.prepareStatement(
				"INSERT INTO do_order_line "+
				" (ol_o_id, ol_line_no, ol_i_id, ol_i_name, ol_quantity, ol_value) "+
				" values (currval('do_order_o_id_seq'), ?, ?, ?, ?, ?)");
			this.stmtOrderReserveInventory = this.dbcon.prepareStatement(
				"UPDATE do_inventory "+
				" SET ii_reserved = ii_reserved + ?, "+
				"     ii_total_sold = ii_total_sold + ?"+
				" WHERE ii_id = ?");
		}

		// ----
		// Select the customer and the number of order lines
		// ----
		var c_id = nurand(this.cfg_aCustomer, 1, this.cfg_nCustomer);
		var o_order_lines = random(5,10);
		var final_order_lines = o_order_lines;
		var o_total_items = 0;
		var o_total_value = 0.0;

		// ----
		// Select the customer for update
		// ----
		this.stmtOrderGetCustomer.setInt(1, c_id);
		res = this.stmtOrderGetCustomer.executeQuery();

		this.debug(1, "new order for customer "+c_id);

		var ol_i_id = [];
		var ol_i_name = [];
		var ol_value = [];
		var ol_quantity = [];
		var ol = 0;
		var i;
		var j;
		var tmp_i_id;
		var tmp_quantity;
		var res;

		// ----
		// Build the wish list of items and quantities
		// ----
		while (ol < o_order_lines) {
			i_id = nurand(this.cfg_aItem, 1, this.cfg_maxItem);
			for (i = 0; i < ol - 1; i++) {
				if (i_id == ol_i_id[i]) {
					i_id = -1;
					break;
				}
			}
			if (i_id < 0) {
				continue;
			}
			ol_i_id[ol] = i_id;
			ol_quantity[ol] = random(1, 50);
			ol++;
		}

		// ----
		// Now sort that list to avoid deadlocks when selecting all the
		// item and inventory lines 
		// ----
		for (i = 0; i < o_order_lines - 1; i++) {
			for (j = i + 1; j < o_order_lines; j++) {
				if (ol_i_id[j] < ol_i_id[i]) {
					tmp_i_id = ol_i_id[i];
					tmp_quantity = ol_quantity[i];
					ol_i_id[i] = ol_i_id[j];
					ol_quantity[i] = ol_quantity[j];
					ol_i_id[j] = tmp_i_id;
					ol_quantity[j] = tmp_quantity;
				}
			}
		}

		// ----
		// Select all those items and their in stock quantities, locking
		// the inventory row to place the reservation
		// ----
		for (ol = 0; ol < o_order_lines; ol++) {
			this.stmtOrderGetItem.setInt(1, ol_i_id[ol]);
			res = this.stmtOrderGetItem.executeQuery();
			if (!res.next()) {
				ol_quantity[ol] = 0;
				final_order_lines--;
				continue;
			}
			if (res.getInt(3) == 0) {
				ol_quantity[ol] = 0;
				final_order_lines--;
				continue;
			}

			// ----
			// Item found and is in stock. Adjust the quantity if less
			// available and remember name and values.
			// ----
			if (res.getInt(3) < ol_quantity[ol]) {
				ol_quantity[ol] = res.getInt(3);
			}
			ol_i_name[ol] = res.getString(1);
			ol_value[ol] = res.getFloat(2) * ol_quantity[ol];
			o_total_items += ol_quantity[ol];
			o_total_value += ol_value[ol];

			this.debug(2, "  order item "+ol_i_id[ol]+
							" quantity "+ol_quantity[ol]);
		}

		// ----
		// It can happen that nothing from the wishlist was available
		// ----
		if (final_order_lines == 0) {
			this.dbcon.rollback();
			return;
		}

		// ----
		// Create the order header
		// ----
		this.stmtOrderCreate.setInt(1, c_id);
		this.stmtOrderCreate.setInt(2, final_order_lines);
		this.stmtOrderCreate.setInt(3, o_total_items);
		this.stmtOrderCreate.setDouble(4, o_total_value);
		this.stmtOrderCreate.executeUpdate();

		// ----
		// Create the order lines and place inventory reservations
		// ----
		ol = 1;
		for (i = 0; i < o_order_lines; i++) {
			if (ol_quantity[i] == 0) {
				continue;
			}
			this.stmtOrderCreateLine.setInt(1, ol);
			this.stmtOrderCreateLine.setInt(2, ol_i_id[i]);
			this.stmtOrderCreateLine.setString(3, ol_i_name[i]);
			this.stmtOrderCreateLine.setInt(4, ol_quantity[i]);
			this.stmtOrderCreateLine.setDouble(5, ol_value[i]);
			this.stmtOrderCreateLine.executeUpdate();

			this.stmtOrderReserveInventory.setInt(1, ol_quantity[i]);
			this.stmtOrderReserveInventory.setInt(2, ol_quantity[i]);
			this.stmtOrderReserveInventory.setInt(3, ol_i_id[i]);
			this.stmtOrderReserveInventory.executeUpdate();

			ol++;
		}

		// ----
		// Update totals in customer
		// ----
		this.stmtOrderUpdateCustomer.setInt(1, 1);
		this.stmtOrderUpdateCustomer.setDouble(2, o_total_value);
		this.stmtOrderUpdateCustomer.setInt(3, c_id);
		this.stmtOrderUpdateCustomer.executeUpdate();

		this.dbcon.commit();
	}


	// ----
	// disorder.trans_orderShip()
	//
	// Selects the 12 oldest, not shipped orders above nSpecial (the
	// first N orders we never ship for high update concurrency on them.
	// Updates the order to status shipped and reducing in_stock and
	// reserved quantity from the inventory.
	// ----
	this.trans_orderShip = function () {
		if (!this.stmtShipGetOrders) {
			this.stmtShipGetOrders = this.dbcon.prepareStatement(
				"SELECT o_id, o_shipped FROM do_order "+
				" WHERE NOT o_shipped AND o_id > ? "+
				" ORDER BY o_id "+
				" LIMIT 12");
			this.stmtShipLockOrder = this.dbcon.prepareStatement(
				"SELECT o_shipped FROM do_order "+
				" WHERE o_id = ? "+
				" FOR UPDATE");
			this.stmtShipUpdateOrder = this.dbcon.prepareStatement(
				"UPDATE do_order SET o_shipped = true "+
				" WHERE o_id = ?");
			this.stmtShipGetLines = this.dbcon.prepareStatement(
				"SELECT ol_line_no, ol_i_id, ol_quantity "+
				" FROM do_order_line "+
				" WHERE ol_o_id = ? "+
				" ORDER BY ol_i_id "+
				" FOR UPDATE");
			this.stmtShipUpdateInventory = this.dbcon.prepareStatement(
				"UPDATE do_inventory "+
				" SET ii_in_stock = ii_in_stock - ?, "+
				"     ii_reserved = ii_reserved - ? "+
				" WHERE ii_id = ?");
			this.stmtShipSerializable = this.dbcon.prepareStatement(
				"SET TRANSACTION ISOLATION LEVEL SERIALIZABLE");
		}

		var res1;

		// ----
		// Get the first 12 unshipped orders that are NOT special
		// ----
		// this.stmtShipSerializable.executeUpdate();
		this.stmtShipGetOrders.setInt(1, this.cfg_nSpecial);
		res1 = this.stmtShipGetOrders.executeQuery();

		while (res1.next()) {
			var res2;
			var o_id;

			o_id = res1.getInt(1);

			this.stmtShipLockOrder.setInt(1, o_id);
			res2 = this.stmtShipLockOrder.executeQuery();

			if (!res2.next()) {
				this.debug(1, "order "+o_id+" not found on lock attempt");
				continue;
			}
			if (res2.getBoolean(1)) {
				this.debug(1, "order "+o_id+" already shipped");
				continue;
			}

			this.debug(1, "shipping order "+o_id);

			// ----
			// Mark the order shipped
			// ----
			this.stmtShipUpdateOrder.setInt(1, o_id);
			this.stmtShipUpdateOrder.executeUpdate();

			// ----
			// Get all the order lines in ol_line_no order, which
			// is also in ascending ol_i_id order.
			// ----
			this.stmtShipGetLines.setInt(1, o_id);
			res2 = this.stmtShipGetLines.executeQuery();
			while (res2.next()) {
				var ol_i_id;
				var ol_quantity;

				ol_i_id = res2.getInt(2);
				ol_quantity = res2.getInt(3);

				this.stmtShipUpdateInventory.setInt(1, ol_quantity);
				this.stmtShipUpdateInventory.setInt(2, ol_quantity);
				this.stmtShipUpdateInventory.setInt(3, ol_i_id);
				this.stmtShipUpdateInventory.executeUpdate();

				this.debug(2, "  ship order "+o_id+
					" item "+ol_i_id+" quantity "+ol_quantity);
			}
			this.dbcon.commit();
		}

		// ----
		// In case we didn't find a single order to ship we need to
		// roll back the transaction.
		// ----
		this.dbcon.rollback();
	}


	// ----
	// disorder.trans_orderStatus()
	//
	// Well, actually this transaction checks the consistency of an
	// order that likely was created or updated recently.
	// ----
	this.trans_orderStatus = function() {
		if (!this.stmtStatusLastOrderOfCustomer) {
			this.stmtStatusLastOrderOfCustomer = this.dbcon.prepareStatement(
				"SELECT max(o_id) FROM do_order WHERE o_c_id = ?");
			this.stmtStatusCheckOrder = this.dbcon.prepareStatement(
				"SELECT o_id, o_order_lines, o_total_items, o_total_value, "+
				"       ol_count, ol_sum_quantity, ol_sum_value, "+
				"       o_order_lines = ol_count, "+
				"       o_total_items = ol_sum_quantity, "+
				"       o_total_value = ol_sum_value "+
				" FROM do_order, (SELECT count(*) as ol_count, "+
				"                        sum(ol_quantity) as ol_sum_quantity, "+
				"                        sum(ol_value) as ol_sum_value "+
				"                  FROM do_order_line "+
				"                  WHERE ol_o_id = ?) AS ol "+
				" where o_id = ?");
		}

		var o_id;
		var res1;

		// ----
		// Select the order to check. In 80% whe are checking a random
		// order in the first special orders that are never shipped but
		// often updated. In 20% we look for the oldest order by a
		// nurand selected customer, which is something most likely
		// recently created.
		// ----
		if (random(0,9) < 8) {
			o_id = random(1, this.cfg_nSpecial);
		} else {
			o_id = 0;
			while (o_id == 0) {
				var c_id = nurand(this.cfg_aCustomer, 1, this.cfg_nCustomer);

				this.stmtStatusLastOrderOfCustomer.setInt(1, c_id);
				res1 = this.stmtStatusLastOrderOfCustomer.executeQuery();

				if (!res1.next()) {
					this.dbcon.rollback();
					return;
				}

				o_id = res1.getInt(1);
			}
		}

		this.stmtStatusCheckOrder.setInt(1, o_id);
		this.stmtStatusCheckOrder.setInt(2, o_id);
		res1 = this.stmtStatusCheckOrder.executeQuery();

		if (!res1.next()) {
			this.dbcon.rollback();
			throw (new Error(this.name+" failed to read order "+o_id+
				" for orderStatus"));
		}

		// ----
		// Look at the specific results of our check
		// ----
		if (!res1.getBoolean(8) || !res1.getBoolean(9) || !res1.getBoolean(10)) {
			this.dbcon.rollback();
			throw (new Error(
				this.name+
				" Consistency problem in order "+res1.getInt(1)+
				" (o_order_lines="+res1.getInt(2)+
				" count(order_lines)="+res1.getInt(5)+
				" o_total_items="+res1.getInt(3)+
				" sum(ol_quantity)="+res1.getInt(6)+
				" o_total_value="+res1.getDouble(4)+
				" sum(ol_value)="+res1.getDouble(7)+")"));
		}

		this.dbcon.rollback();

		this.debug(1, "status of order "+o_id+" checked");
	}


	// ----
	// disorder.trans_restock()
	//
	// Restock all items that are still in production and which have
	// less than 200 of available quantity.
	// ----
	this.trans_restock = function () {
		if (!this.stmtRestock) {
			this.stmtRestock = this.dbcon.prepareStatement(
				"INSERT INTO do_restock "+
				" (r_i_id, r_quantity) "+
				" values (?, ?)");
			this.stmtRestockSelectInventory = this.dbcon.prepareStatement(
				"SELECT ii_id FROM do_inventory "+
				" JOIN do_item ON i_id = ii_id "+
				" WHERE ii_id = ? "+
				" AND i_in_production "+
				" AND ii_in_stock - ii_reserved < 200 "+
				" FOR UPDATE OF do_inventory");
			this.stmtRestockUpdateInventory = this.dbcon.prepareStatement(
				"UPDATE do_inventory "+
				" SET ii_in_stock = ii_in_stock + ? "+
				" WHERE ii_id = ?");
			this.stmtRestockOutOfProduction = this.dbcon.prepareStatement(
				"UPDATE do_item "+
				" SET i_in_production = false "+
				" WHERE i_id = ?");
		}

		var stmt1 = this.dbcon.createStatement();

		// ----
		// First adjust internal statistics that we need to take
		// items out of production.
		// ----
		this.trans_adjustCounts();

		// ----
		// Get all items that have less than 200 available quantity.
		// We cannot lock them at this point because in order to avoid
		// deadlocks, all transactions lock items always in ascending
		// order and there is no way to force the order at this point.
		// ----
		var items = stmt1.executeQuery(
				"SELECT ii_id, i_critical FROM do_inventory "+
				" JOIN do_item ON i_id = ii_id "+
				" WHERE i_in_production "+
				" AND ii_in_stock - ii_reserved < 200 "+
				" ORDER BY ii_id "+
				" LIMIT 20");

		while (items.next()) {
			// ----
			// Get the next item locked. This may not select the row
			// because it is possible that some other transaction has
			// restocked or an order cancelation has happened.
			// ----
			var i_id = items.getInt(1);
			var i_critical = items.getBoolean(2);
			this.stmtRestockSelectInventory.setInt(1, i_id);
			var res = this.stmtRestockSelectInventory.executeQuery();
			if (!res.next()) {
				continue;
			}

			// ----
			// If the current number of items is larger than the configured
			// number, we let some get out of production.
			// ----
			if (this.cfg_numItem > this.cfg_nItem && 
					i_critical == false &&
					random(1, 10) <= 2) {
				this.stmtRestockOutOfProduction.setInt(1, i_id);
				this.stmtRestockOutOfProduction.executeUpdate();
				continue;
			}

			// ----
			// Restock
			// ----
			var r_quantity = random(2,6) * 50;

			this.stmtRestock.setInt(1, i_id);
			this.stmtRestock.setInt(2, r_quantity);
			this.stmtRestock.executeUpdate();

			this.stmtRestockUpdateInventory.setInt(1, r_quantity);
			this.stmtRestockUpdateInventory.setInt(2, i_id);
			this.stmtRestockUpdateInventory.executeUpdate();

			this.debug(1, "restocked item "+i_id+" with "+r_quantity+" units");
		}
		this.dbcon.commit();
	}

	// ----
	// disorder.trans_orderUpdate()
	// 
	// Change the quantities in some order lines of an unshipped order.
	// This also requires to update the totals in the order head, the
	// customer and the inventory.
	// ----
	this.trans_orderUpdate = function () {
		if (!this.stmtUpdateGetCustomerSpecial) {
			this.stmtUpdateGetCustomerSpecial = this.dbcon.prepareStatement(
				"SELECT c_id "+
				" FROM do_order JOIN do_customer ON c_id = o_c_id "+
				" WHERE o_id = ? "+
				" FOR UPDATE OF do_customer");
			this.stmtUpdateGetCustomer = this.dbcon.prepareStatement(
				"SELECT c_id FROM do_customer "+
				" WHERE c_id = ?");
			this.stmtUpdateGetLastOrder = this.dbcon.prepareStatement(
				"SELECT max(o_id) FROM do_order "+
				" WHERE o_c_id = ? "+
				" AND NOT o_shipped");
			this.stmtUpdateGetOrder = this.dbcon.prepareStatement(
				"SELECT o_order_lines "+
				" FROM do_order "+
				" WHERE o_id = ? "+
				" AND NOT o_shipped "+
				" FOR UPDATE OF do_order");
			this.stmtUpdateGetOrderLine = this.dbcon.prepareStatement(
				"SELECT ol_i_id, ol_line_no, ol_quantity "+
				" FROM do_order_line "+
				" WHERE ol_o_id = ? "+
				" ORDER BY ol_i_id "+
				" FOR UPDATE OF do_order_line");
			this.stmtUpdateGetInventory = this.dbcon.prepareStatement(
				"SELECT i_name, i_price, "+
				"     ii_in_stock - ii_reserved as ii_available"+
				" FROM do_item "+
				" JOIN do_inventory ON ii_id = i_id "+
				" WHERE i_id = ? "+
				" FOR UPDATE OF do_inventory");
			this.stmtUpdateOrderLine = this.dbcon.prepareStatement(
				"UPDATE do_order_line "+
				" SET ol_quantity = ?, "+
				"     ol_value = ? "+
				" WHERE ol_o_id = ? "+
				" AND ol_line_no = ?");
			this.stmtUpdateInventory = this.dbcon.prepareStatement(
				"UPDATE do_inventory "+
				" SET ii_reserved = ii_reserved + ?, "+
				"     ii_total_sold = ii_total_sold + ? "+
				" WHERE ii_id = ?");
			this.stmtUpdateOrder = this.dbcon.prepareStatement(
				"UPDATE do_order "+
				" SET o_total_items = o_total_items + ?, "+
				"     o_total_value = o_total_value + ? "+
				" WHERE o_id = ?");
			this.stmtUpdateCustomer = this.dbcon.prepareStatement(
				"UPDATE do_customer "+
				" SET c_total_value = c_total_value + ? "+
				" WHERE c_id = ?");
		}

		var c_id;
		var o_id;
		var res1;
		var res2;

		// ----
		// Select the order to modify. Lock the customer for update
		// later while doing so.
		// ----
		if (random(0,9) < 8) {
			o_id = random(1, this.cfg_nSpecial);

			this.stmtUpdateGetCustomerSpecial.setInt(1, o_id);
			res1 = this.stmtUpdateGetCustomerSpecial.executeQuery();
			if (!res1.next()) {
				this.dbcon.rollback();
				return;
			}
			c_id = res1.getInt(1);
		} else {
			var c_id = nurand(this.cfg_aCustomer, 1, this.cfg_nCustomer);

			this.stmtUpdateGetCustomer.setInt(1, c_id);
			this.stmtUpdateGetCustomer.executeQuery();

			this.stmtUpdateGetLastOrder.setInt(1, c_id);
			res1 = this.stmtUpdateGetLastOrder.executeQuery();

			if (!res1.next()) {
				this.dbcon.rollback();
				return;
			}

			o_id = res1.getInt(1);
		}

		// ----
		// Select the order for update. This can fail if the order
		// was already shipped by a concurrent transaction.
		// ----
		this.stmtUpdateGetOrder.setInt(1, o_id);
		res1 = this.stmtUpdateGetOrder.executeQuery();
		if (!res1.next()) {
			this.dbcon.rollback();
			return;
		}
		var o_order_lines = res1.getInt(1);
		var diff_items = 0;
		var diff_value = 0.0;

		this.debug(1, "update order "+o_id);

		this.stmtUpdateGetOrderLine.setInt(1, o_id);
		res1 = this.stmtUpdateGetOrderLine.executeQuery();
		while (res1.next()) {
			if (random(0, 2) != 0)
				continue;

			var ol_i_id = res1.getInt(1);
			var ol = res1.getInt(2);
			var ol_quantity = res1.getInt(3);

			this.stmtUpdateGetInventory.setInt(1, ol_i_id);
			res2 = this.stmtUpdateGetInventory.executeQuery();
			res2.next();
			var i_price = res2.getDouble(2);
			var ii_available = res2.getInt(3);

			// ----
			// Decide on a new quantity. This can fail if the old
			// quantity was 1 and no more inventory is available.
			// ----
			var new_quantity = ol_quantity;
			while (new_quantity == ol_quantity) {
				new_quantity = random(1, 50);
			}
			if (ii_available + ol_quantity < new_quantity) {
				new_quantity = ii_available + ol_quantity;
			}
			if (new_quantity == ol_quantity) {
				new_quantity -= 1;
				if (new_quantity == 0) {
					continue;
				}
			}
			var diff = new_quantity - ol_quantity;

			this.debug(2, "  change quantity of ol "+ol+
				" (item "+ol_i_id+") by "+diff+" from "+
				ol_quantity+" to "+new_quantity);

			// ----
			// Keep track of the order level changes in total.
			// ----
			diff_items += diff;
			diff_value += i_price * diff;

			// ----
			// Update the order line and the inventory
			// ----
			this.stmtUpdateOrderLine.setInt(1, new_quantity);
			this.stmtUpdateOrderLine.setDouble(2, i_price * new_quantity);
			this.stmtUpdateOrderLine.setInt(3, o_id);
			this.stmtUpdateOrderLine.setInt(4, ol);
			this.stmtUpdateOrderLine.executeUpdate();

			this.stmtUpdateInventory.setInt(1, diff);
			this.stmtUpdateInventory.setInt(2, diff);
			this.stmtUpdateInventory.setInt(3, ol_i_id);
			this.stmtUpdateInventory.executeUpdate();
		}

		// ----
		// Update the Order and Customer total values
		// ----
		this.stmtUpdateOrder.setInt(1, diff_items);
		this.stmtUpdateOrder.setDouble(2, diff_value);
		this.stmtUpdateOrder.setInt(3, o_id)
		this.stmtUpdateOrder.executeUpdate();

		this.stmtUpdateCustomer.setDouble(1, diff_value);
		this.stmtUpdateCustomer.setInt(2, c_id);
		this.stmtUpdateCustomer.executeUpdate();

		this.dbcon.commit();

		this.debug(1, "order "+o_id+" updated");
	}

	// ----
	// disorder.trans_itemCreate()
	//
	// Create a new item.
	// ----
	this.trans_itemCreate = function () {
		if (!this.stmtItemCreate) {
			this.stmtItemCreate = this.dbcon.prepareStatement(
				"INSERT INTO do_item "+
				" (i_name, i_price, i_in_production, i_critical) "+
				" VALUES ("+
				"   'Item ' || digsyl(random(0, 99999999), 8), "+
				"   random(99, 9999)::numeric / 100.0, "+
				"   true, false)");
		}

		this.stmtItemCreate.executeUpdate();
		this.dbcon.commit();

		this.debug(1, "item created");
	}

	// ----
	// disorder.trans_itemDelete()
	//
	// Delete items that are no longer in production and that
	// have no inventory left. Note that a lot of cleanup work
	// will be done by foreign key constraints.
	// ----
	this.trans_itemDelete = function () {
		if (!this.stmtItemDelete) {
			this.stmtItemDelete = this.dbcon.prepareStatement(
				"DELETE FROM do_item "+
				" WHERE i_in_production = false "+
				" AND (SELECT ii_in_stock FROM do_inventory "+
				"      WHERE ii_id = i_id) = 0");
		}

		this.stmtItemDelete.executeUpdate();
		this.dbcon.commit();

		this.debug(1, "item delete attempted");
	}

	// ----
	// disorder.trans_checkItem()
	//
	//	Select the top ten items that have been ordered by the most
	//	distinct customers. Then do check of the in_stock, reserved
	//	and total_sold counts in the inventory.
	// ----
	this.trans_checkItem = function () {
		if (!this.stmtCheckItemSerializable) {
			this.stmtCheckItemSerializable = this.dbcon.prepareStatement(
				"SET TRANSACTION ISOLATION LEVEL SERIALIZABLE");
			this.stmtCheckItemGetTopTen = this.dbcon.prepareStatement(
				"SELECT i_id, count(DISTINCT o_c_id) "+
				" FROM do_item "+
				" JOIN (SELECT ol_i_id, o_c_id FROM do_order_line "+
				"        JOIN do_order ON o_id = ol_o_id) AS oc "+
				"      ON ol_i_id = i_id "+
				" GROUP BY 1 ORDER BY 2 DESC "+
				" LIMIT 10");
			this.stmtCheckItemInStock = this.dbcon.prepareStatement(
				"SELECT ii_in_stock, r_quant - coalesce(s_quant, 0) "+
				" FROM do_inventory "+
				" LEFT JOIN (SELECT r_i_id, sum(r_quantity) as r_quant "+
				"             FROM do_restock "+
				"             GROUP BY r_i_id) AS restock ON r_i_id = ii_id "+
				" LEFT JOIN (SELECT ol_i_id, sum(ol_quantity) AS s_quant "+
				"             FROM do_order_line "+
				"             JOIN do_order ON o_id = ol_o_id "+
				"             WHERE o_shipped "+
				"             GROUP BY ol_i_id) AS shipped ON ol_i_id = ii_id "+
				" WHERE ii_id = ?");
			this.stmtCheckItemReserved = this.dbcon.prepareStatement(
				"SELECT ii_reserved, ol_quantity AS should_be "+
				" FROM do_inventory "+
				" LEFT JOIN (SELECT ol_i_id, sum(ol_quantity) AS ol_quantity "+
				"             FROM do_order_line "+
				"             JOIN do_order ON o_id = ol_o_id "+
				"             WHERE NOT o_shipped "+
				"             GROUP BY ol_i_id) AS ordered ON ol_i_id = ii_id "+
				" WHERE ii_id = ?");
			this.stmtCheckItemTotalSold = this.dbcon.prepareStatement(
				"SELECT ii_total_sold, sum(ol_quantity) AS should_be "+
				" FROM do_inventory "+
				" LEFT JOIN do_order_line ON ol_i_id = ii_id "+
				" WHERE ii_id = ? "+
				" GROUP BY ii_total_sold ");
		}

		this.stmtCheckItemSerializable.executeUpdate();

		var errors = 0;
		var errmsg = "";
		var res1 = this.stmtCheckItemGetTopTen.executeQuery();
		while (res1.next()) {
			var i_id = res1.getInt(1);

			this.debug(1, "checking item "+i_id);

			// ----
			// The in_stock column must be equal to the sum of all
			// restockings minus the sum of all shipped order quantities.
			// ----
			this.stmtCheckItemInStock.setInt(1, i_id);
			var res2 = this.stmtCheckItemInStock.executeQuery();
			if (!res2.next()) {
				errors++;
				errmsg += "Item "+i_id+" cannot retrieve in_stock values\n";
				continue;
			}
			var in_stock = res2.getInt(1);
			var should_be = res2.getInt(2);
			if (in_stock != should_be) {
				errors++;
				errmsg += "Item "+i_id+" in_stock="+in_stock+
						  " should be "+should_be+"\n";
			}

			// ----
			// The reserved column must be equal to the sum of all
			// unshipped order quantities.
			// ----
			this.stmtCheckItemReserved.setInt(1, i_id);
			res2 = this.stmtCheckItemReserved.executeQuery();
			if (!res2.next()) {
				errors++;
				errmsg += "Item "+i_id+" cannot retrieve reserved values\n";
				continue;
			}
			var reserved = res2.getInt(1);
			should_be = res2.getInt(2);
			if (reserved != should_be) {
				errors++;
				errmsg += "Item "+i_id+" reserved="+reserved+
						  " should be "+should_be+"\n";
			}

			// ----
			// The total_sold column must be equal to the sum of all
			// order quantities.
			// ----
			this.stmtCheckItemTotalSold.setInt(1, i_id);
			res2 = this.stmtCheckItemTotalSold.executeQuery();
			if (!res2.next()) {
				errors++;
				errmsg += "Item "+i_id+" cannot retrieve total_sold values\n";
				continue;
			}
			var total_sold = res2.getInt(1);
			should_be = res2.getInt(2);
			if (total_sold != should_be) {
				errors++;
				errmsg += "Item "+i_id+" total_sold="+total_sold+
						  " should be "+should_be+"\n";
			}
		}

		this.dbcon.commit();

		if (errors > 0) {
			throw(new Error(errmsg));
		}
	}

	this.trans_checkCustomer = function () {
		if (!this.stmtCheckCustomer) {
			this.stmtCheckCustomer = this.dbcon.prepareStatement(
				"SELECT c_total_orders, count(o_id) as count_order, "+
				"       c_total_value, sum(o_total_value) as sum_order_value "+
				" FROM do_customer "+
				" LEFT JOIN do_order ON o_c_id = c_id "+
				" WHERE c_id = ? "+
				" GROUP BY c_total_orders, c_total_value");
		}

		var c_id = nurand(this.cfg_aCustomer, 1, this.cfg_nCustomer);
		this.debug(1, "checking customer "+c_id);

		this.stmtCheckCustomer.setInt(1, c_id);
		var res = this.stmtCheckCustomer.executeQuery();
		if (!res.next()) {
			this.dbcon.rollback();
			throw (new Error("failed to check customer "+c_id+
					" - result set empty"));
		}

		var c_num = res.getInt(1);
		var o_num = res.getInt(2);
		var c_val = res.getDouble(3);
		var o_val = res.getDouble(4);

		this.dbcon.commit();

		var errors = 0;
		var errmsg = "";
		if (c_num != o_num) {
			errors++;
			errmsg += "customer "+c_id+
				" c_total_orders="+c_num+
				" should be "+o_num+"\n";
		}
		if (c_val != o_val) {
			errors++;
			errmsg += "customer "+c_id+
				" c_total_value="+c_val+
				" should be "+o_val+"\n";
		}

		if (errors > 0) {
			throw (new Error(errmsg));
		}
	}

} // Class disorder


