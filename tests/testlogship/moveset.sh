testname=$1
echo "
  LOCK SET ( ID = 1, ORIGIN = 1 );
  MOVE SET ( ID = 1, OLD ORIGIN = 1, NEW ORIGIN = 3 );
"
