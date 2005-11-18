
err()
{
    exitval=$1
    shift
    echo 1>&2 "$0: ERROR: $*"
    numerrors=`expr ${numerrors} + 1`
    exit $exitval
}

warn()
{
	shift
	echo 1>&2 "$0: WARNING: $*"
	numerrors=`expr ${numerrors} + 1`
}

status()
{
	if [ "x$SHLVL" != "x" ]; then
      case `uname` in
        MINGW32*)
	      echo "$*"
          ;;
        *)
          echo "$*" > `tty`
          ;;
      esac
    fi
}

_check_pid()
{
	if [ $# -ne 3 ]; then
	  err 3 'USAGE: _check_pid procname pid ppid'
	fi
	case `uname` in
        FreeBSD)
        	_psargs="-j"
            _fp_args='_user _pid _ppid _pgid _sess _jobc _stat _tt _time _command'
            ;;
        MINGW32*)
            _psargs="-ef"
            _fp_args='_user _pid _ppid _tt _time _command'
            ;;
        *)
            _psargs="-ef"
            _fp_args='_user _pid _ppid _c _stat _tt _time _command'
            ;;
	esac

	_procname=$1
	_qpid=$2
	_qppid=$3

	_proccheck=
	_fp_match=

	if [ ! $_qpid -gt 0 ]; then
		err 3 'not a valid PID'
	fi
	  _procnamebn=${_procname##*/}
	  _fp_match="case \"\$_command\" in
	    *${_procname}*)"

	eval _proccheck='
		ps 2>/dev/null '"$_psargs"' |
		while read '"$_fp_args"'; do
			case "$_pid" in
			  PID)
				continue ;;
			esac ; '"$_fp_match"'
			  	if [ "$_pid" -eq  "$_qpid" ]; then 
					echo -n "$_qpid" ;
				fi
			  ;;
			esac
		done'
}

random_number()
{
  if [ $# -ne 2 ]; then
    err 3 'USAGE: random_number lowerbound upperbound'
  fi

  _lowerbound=$1
  _upperbound=$2

  case `uname` in
  FreeBSD)
    rannum=`jot -r 1 ${_lowerbound} ${_upperbound}`
    ;;
  AIX|MINGW32*)
    rannum=`echo | awk -v _upperbound=${_upperbound} -v _lowerbound=${_lowerbound} '{srand(); printf "%.0d\n", (rand() * _upperbound) + _lowerbound;}'` 
    ;;
  Linux)
    rannum=`echo | awk -v _upperbound=${_upperbound} -v _lowerbound=${_lowerbound} '{srand(); printf "%.0d\n", (rand() * _upperbound) + _lowerbound;}'`
    ;;
  SunOS)
    rannum=`echo | nawk -v _upperbound=${_upperbound} -v _lowerbound=${_lowerbound} '{srand(); printf "%.0d\n", (rand() * _upperbound) + _lowerbound;}'`
    ;;
  *)
    originnode=${ORIGINNODE:-"1"}
    eval odb=\$DB${originnode}
    eval ohost=\$HOST${originnode}
    eval ouser=\$USER${originnode}
    eval _upperbound=${_upperbound}    
    eval _lowerbound=${_lowerbound}
    eval opath=\$PGBINDIR${originnode}
    rannum=`${opath}/psql -c "SELECT round(random()* ${_upperbound} + ${_lowerbound});" -t -A -h ${ohost} ${odb} ${ouser}`
  ;;
  esac
  echo ${rannum}
}

random_string()
{
  if [ $# -ne 1 ]; then
    err 3 'USAGE: random_string length'
  fi

  _length=$1
  case `uname` in
  FreeBSD)
    ranstring=`jot -r -c ${_length} a Z | rs -g 0 ${_length}`
    ;;
  Linux|AIX|MINGW32*)
    ranstring=`echo | awk -v _length=${_length} '{srand(); {for (i=0; i<= _length ; i++) printf "%c", (rand() * (122-48))+48};}'`
    ;;
  SunOS)
    ranstring=`echo | nawk -v _length=${_length} '{srand(); {for (i=0; i<= _length ; i++) printf "%c", (rand() * (122-48))+48};}'`
    ;;
  *)
    originnode=${ORIGINNODE:-"1"}
    eval odb=\$DB${originnode}
    eval ohost=\$HOST${originnode}
    eval ouser=\$USER${originnode}
    eval opath=\$PGBINDIR${originnode}
    alias=${_length}
    while : ; do
      ranstring=${ranstring}`${opath}/psql -c "SELECT chr(round(random()*((122-48))+48)::int4);" -t -A -h ${ohost} ${odb} ${ouser}`
      if [ ${alias} -ge ${_length} ]; then
        break;
      else
        alias=$((${alias} + 1))
      fi
    done;
  ;;
  esac

  echo ${ranstring}
}

random_az()
{
  if [ $# -ne 1 ]; then
    err 3 'USAGE: random_string length'
  fi

  _length=$1
  case `uname` in
  FreeBSD)
    ranstring=`jot -r -c ${_length} a Z | rs -g 0 ${_length}`
    ;;
  AIX|MINGW32*)
    ranstring=`echo | awk -v _length=${_length} '{srand(); {for (i=0; i<= _length ; i++) printf "%c", (rand() * ((122)-97))+97};}'`
    ;;
  SunOS)
    ranstring=`echo | nawk -v _length=${_length} '{srand(); {for (i=0; i<= _length ; i++) printf "%c", (rand() * ((122)-97))+97};}'`
    ;;
  *)
    originnode=${ORIGINNODE:-"1"}
    eval odb=\$DB${originnode}
    eval ohost=\$HOST${originnode}
    eval ouser=\$USER${originnode}
    alias=${_length}
    while : ; do
      ranstring=${ranstring}`psql -c "SELECT chr(round(random()*(((122)-97))+97)::int4);" -t -A -h ${ohost} ${odb} ${ouser}`
      if [ ${alias} -ge ${_length} ]; then
        break;
      else
        alias=$((${alias} + 1))
      fi
    done;
  ;;
  esac

  echo ${ranstring}
}
