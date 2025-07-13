#!/bin/bash

set -e

# ----------------------help----------------------
# APP VERSION 1.0.1
# HELP (this message)
# --------------system configuration--------------
# SET_NAME XXXXXX (len max 58)
# GET_NAME (8)
# GET_UID (7)
# CONFIG_SAVE (11)
# CONFIG_DUMP (11)
# CONFIG_LOAD (11)
# CONFIG_DEFAULT (14)
# ---------------gpio configuration---------------
# gpio configuration
# INIT NAME MODE PULL
#   NAME (4 bytes)
#         IO01, IO15, IO33, ...
#   MODE (3 bytes)
#         INP   (Input Floating)
#         ADC   (Input ADC IO01, IO02)
#         OPP   (Output Push Pull Mode)
#         OOD   (Output Open Drain Mode)
#   PULL (2 bytes)
#         NO   (No Pull-up or Pull-down)
#         UP   (Pull-up)
#         DO   (Pull-down)
# sample
# INIT IO02 OPP NO (4 bytes)
# -----------------gpio read/write----------------
# GET NAME
# SET NAME VALUE (10 bytes)


tty=
value=

#speed=115200
speed=230400

LED_BLUE="IO15"
BT0="IO00"

led_init()
{
    uart_send "INIT ${1} OPP NO"
}

led_on()
{
    uart_send "SET ${1} 1"
    #uart_send "GET ${1}"
}

led_off()
{
    uart_send "SET ${1} 0"
    #uart_send "GET ${1}"
}

read_button()
{
    echo -n "button ${1}:"
    uart_send "GET ${1}"
}

uart_open()
{
    local uart="${1}"
    if ! stty -F "${uart}" ispeed "${speed}" ospeed "${speed}" cs8 raw -echo ; then
        return 1
    fi

    #stty --all -F ${uart}

    ## open file descriptor for uart
    exec 101< "${uart}"
    exec 102> "${uart}"
}

uart_close()
{
    ## close file descriptor for uart
    exec 101<&-
    exec 102>&-
}


uart_send()
{
    value=
    local verbose=true
    #echo "${1}"
    if [ "${2}" == false ] ; then
     verbose=false
    fi
    printf "%s\r" "${1}" >& 102
    sleep 0.02
    if ! read -r -n 200 -t 2 -u 101 line ; then
       return 1
    fi
    if ! echo "$line" | grep -q '\*OK\*'  ; then
        if $verbose ;then
            echo "error : $line"
        fi
        return 1
    fi
    if echo "$line" | grep -q "value"  ; then
        value=$( echo "$line" | awk -F ":" '{ $1 = "" ; print $0 }')
        if $verbose ;then
            echo "$value"
        fi
    fi

    return 0
}


detect()
{
  local uart="unknow"
  local uid=
  local name=

  for t in /dev/ttyACM* ; do
    echo "try : ${t}"
    if ! uart_open "${t}" ; then
        continue
    fi
    if ! uart_send "GET_UID" "false" ; then
        continue
    fi

    uart="${t}"
    uid="${value}"

    if ! uart_send "GET_NAME" "false" ; then
         continue
    fi
    name=$value
    break
  done

  if [ "$uart" != "unknow" ] ; then
   tty=$uart
   echo "found device name $name : UID $uid on $tty "
   return 0
  fi

  echo "device not found"
  exit 1

}

detect

echo "tty ${tty}"

uart_open "${tty}"

#config
led_init "${LED_BLUE}"

read_button "${BT0}"

#demo
loop=25
echo "flash the LED ${loop} times"
while [ "${loop}" -ge 0 ]; do
  led_on "${LED_BLUE}"
  sleep 0.08
  led_off "${LED_BLUE}"
  ((loop--))
done

uart_close

