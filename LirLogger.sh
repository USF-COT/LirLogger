#!/bin/sh
#
# LirLogger        Startup script for LirLogger
#
# chkconfig: - 85 15
# processname: LirLogger
# pidfile: /var/run/LirLogger.pid
# description: LirLogger is a remotely configurable service that controls 1 camera and multiple serial sensors
#
### BEGIN INIT INFO
# Provides: LirLogger
# Required-Start: $local_fs $remote_fs $network
# Required-Stop: $local_fs $remote_fs $network
# Default-Start: 2 3 4 5
# Default-Stop: 0 1 6
# Short-Description: start and stop LirLogger
### END INIT INFO

# Source function library.
. /etc/rc.d/init.d/functions

if [ -f /etc/sysconfig/LirLogger ]; then
    . /etc/sysconfig/LirLogger
fi

prog=LirLogger
lirlogger=${LIRLOGGER-/usr/local/bin/LirLogger}
pidfile=${PIDFILE-/var/run/LirLogger.pid}
pleora_env="/opt/pleora/ebus_sdk/bin/set_puregev_env"
SLEEPMSEC=100000
RETVAL=0

start() {
    echo -n $"Starting $prog: "

    source ${pleora_env}
    daemon --pidfile=${pidfile} ${lirlogger}
    return 1
}

stop() {
    echo -n $"Stopping $prog: "
    killproc ${prog}
    RETVAL=$?
    echo
    [ $RETVAL = 0 ] && rm -f ${pidfile}
}

reload() {
    echo -n $"Reloading $prog: "
    killproc -p ${pidfile} ${prog} -HUP
    RETVAL=$?
    echo
}

upgrade() {
    oldbinpidfile=${pidfile}.oldbin

    configtest -q || return 6
    echo -n $"Staring new master $prog: "
    killproc -p ${pidfile} ${prog} -USR2
    RETVAL=$?
    echo
    /bin/usleep $SLEEPMSEC
    if [ -f ${oldbinpidfile} -a -f ${pidfile} ]; then
        echo -n $"Graceful shutdown of old $prog: "
        killproc -p ${oldbinpidfile} ${prog} -QUIT
        RETVAL=$?
        echo 
    else
        echo $"Upgrade failed!"
        return 1
    fi
}

rh_status() {
    status -p ${pidfile} ${lirlogger}
}

# See how we were called.
case "$1" in
    start)
        rh_status >/dev/null 2>&1 && exit 0
        start
        ;;
    stop)
        stop
        ;;
    status)
        rh_status
        RETVAL=$?
        ;;
    restart)
        stop
        start
        ;;
    upgrade)
        upgrade
        ;;
    condrestart|try-restart)
        if rh_status >/dev/null 2>&1; then
            stop
            start
        fi
        ;;
    force-reload|reload)
        reload
        ;;
    *)
        echo $"Usage: $prog {start|stop|restart|condrestart|try-restart|force-reload|upgrade|reload|status|help}"
        RETVAL=2
esac

exit $RETVAL
