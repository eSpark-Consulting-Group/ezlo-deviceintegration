#!/bin/sh

DRIVER_ADDR=0x45
RED="red"
GREEN="green"
BLUE="blue"

CURRENT_5mA=0x01
MODE_PWM=0x10
MODE_ONESHOT=0x00

TIME_130_MS=0x00
TIME_260_MS=0x01
TIME_520_MS=0x02
TIME_1040_MS=0x03
TIME_2080_MS=0x04
TIME_4160_MS=0x05
TIME_8320_MS=0x06
TIME_16640_MS=0x07
NO_DELAY=0xff # Special value for T0 = 0ms

LED_LOCK="/tmp/.led_lock"
LED_ENABLED="/tmp/.led_enabled"

reset_chip()
{
    i2cset -y 0 $1 0x00 0x55
}

critical_error()
{
    >&2 echo "$1"
    reset_chip $DRIVER_ADDR
    exit 1
}

enable_chip()
{
    i2cset -y 0 $DRIVER_ADDR 0x01 0x01
}

check_color()
{
    if [ "$1" != "$RED" ] && [ "$1" != "$GREEN" ] && [ "$1" != "$BLUE" ];
    then
            critical_error "Error! Color undefined: $1"
    fi
}

color_to_register()
{
    case $1 in
        $GREEN)
            echo "$2"
            ;;
        $RED)
            echo "$3"
            ;;
        $BLUE)
            echo "$4"
            ;;
        *)
            critical_error "Error! Color undefined: $1"
            ;;
    esac
}

set_mode()
{
    check_color $1
    MODE_REGISTER="$( color_to_register $1 0x31 0x32 0x33 )"
    MODE=$2

    if [ "$MODE" != "$MODE_PWM" ] && [ "$MODE" != "$MODE_ONESHOT" ];
    then
            critical_error "Error! mode undefined: $MODE"
    fi

    i2cset -y 0 $DRIVER_ADDR $MODE_REGISTER $( printf "0x%x\n" $(( $MODE | $CURRENT_5mA )) )
}

set_brightness()
{
    check_color $1
    BR_REGISTER="$( color_to_register $1 0x34 0x35 0x36 )"
    VALUE=$2
    i2cset -y 0 $DRIVER_ADDR $BR_REGISTER $VALUE
}

enable_channel()
{
    CONTROL_REGISTER=0x30
    CHANNEL_MASK=0x0
    while [ $# -gt 0 ]
    do
        check_color $1
        CHANNEL_MASK=$(( $CHANNEL_MASK | $( color_to_register $1 0x01 0x02 0x04 ) ))
        shift
    done
    CHANNEL_MASK=$( printf "0x%x\n" $CHANNEL_MASK )

    i2cset -y 0 $DRIVER_ADDR $CONTROL_REGISTER $CHANNEL_MASK
}

set_t1_t2_time()
{
    check_color $1
    T1=$2
    T2=$3
    T1_T2_REGISTER="$( color_to_register $1 0x37 0x3A 0x3D )"
    VALUE="$( printf "0x%x\n" $(( $(( $T1 << 4 )) | $T2 )) )"
    i2cset -y 0 $DRIVER_ADDR $T1_T2_REGISTER $VALUE
}

set_t3_t4_time()
{
    check_color $1
    T3=$2
    T4=$3
    T3_T4_REGISTER="$( color_to_register $1 0x38 0x3B 0x3E )"
    VALUE="$( printf "0x%x\n" $(( $(( $T3 << 4 )) | $T4 )) )"
    i2cset -y 0 $DRIVER_ADDR $T3_T4_REGISTER $VALUE
}

set_t0_and_repeat()
{
    check_color $1
    T0=$2
    REPEAT=$3
    T0_REPEAT_REGISTER="$( color_to_register $1 0x39 0x3C 0x3F )"
    if [ "$T0" = "$NO_DELAY" ];
    then
        T0=0x0
    else
        T0=$(( $T0 + 1 ))
    fi

    VALUE="$( printf "0x%x\n" $(( $(( $T0 << 4 )) | $(( $REPEAT & 0x7 )) )) )"
    i2cset -y 0 $DRIVER_ADDR $T0_REPEAT_REGISTER $VALUE
}

led_on()
{
    reset_chip $DRIVER_ADDR
    enable_chip
    set_mode $GREEN $MODE_ONESHOT
    set_mode $RED $MODE_ONESHOT
    set_brightness $GREEN 0xff
    set_brightness $RED 0xff
    enable_channel $GREEN $RED
}

led_off()
{
    reset_chip $DRIVER_ADDR
}

#pwm_example()
#{
#    reset_chip $DRIVER_ADDR
#    enable_chip
#    set_mode $GREEN $MODE_ONESHOT
#    set_mode $RED $MODE_PWM
#    set_brightness $GREEN 0x99
#    set_brightness $RED 0xff
#    set_t1_t2_time $RED $TIME_2080_MS $TIME_1040_MS
#    set_t3_t4_time $RED $TIME_4160_MS $TIME_130_MS
#    enable_channel $GREEN $RED
#}

check_led_addr()
{
    ADDR_REGISTER="0x77"
    BUS_NUM="0"

    if i2cget -y $BUS_NUM $DRIVER_ADDR $ADDR_REGISTER &>/dev/null ; then
        return 0
    fi

    echo "LED not found at ($DRIVER_ADDR). Start searching ..."

    reg_mask="$(( 1<<7 ))"
    fixed_reg_value=$( printf "0x%x" $(( $reg_mask | $DRIVER_ADDR )) )

    addr_list="$( i2cdetect -a -y $BUS_NUM | tail -n +2 | cut -c 5- | tr '\n-' ' ' | tr -s ' ' )"

    for addr in $( echo ${addr_list} )
    do
        addr="0x$addr"

        reg_value="$( i2cget -a -y $BUS_NUM $addr $ADDR_REGISTER 2>/dev/null )"
        if [ "$?" = "0" ] && [ "$reg_value" = "$addr" ]; then
            echo "LED found at ($addr). Moving to default address ($DRIVER_ADDR)"
            i2cset -a -y $BUS_NUM $addr $ADDR_REGISTER $fixed_reg_value
            return 0
        fi
    done

    echo "LED not found on bus"
    return 1
}

fix_addr()
{
    check_led_addr
    exit $?
}

start()
{
    check_led_addr
    (
    flock 200
    led_on
    touch "$LED_ENABLED"
    ) 200>"$LED_LOCK"
}

indicate_wait()
{
    check_led_addr
    (
    flock 200
    reset_chip $DRIVER_ADDR
    enable_chip
    set_mode $BLUE $MODE_ONESHOT
    set_brightness $BLUE 0xff
    enable_channel $BLUE
    ) 200>"$LED_LOCK"
}

indicate_progress()
{
    check_led_addr
    (
    flock 200
    reset_chip $DRIVER_ADDR
    enable_chip
    set_mode $BLUE $MODE_PWM
    set_brightness $BLUE 0xff
    set_t1_t2_time $BLUE $TIME_260_MS $TIME_260_MS
    set_t3_t4_time $BLUE $TIME_260_MS $TIME_260_MS
    enable_channel $BLUE
    ) 200>"$LED_LOCK"
}

indicate_error()
{
    check_led_addr
    (
    flock 200
    reset_chip $DRIVER_ADDR
    enable_chip
    set_mode $RED $MODE_PWM
    set_brightness $RED 0xff
    set_t1_t2_time $RED $TIME_260_MS $TIME_260_MS
    set_t3_t4_time $RED $TIME_260_MS $TIME_260_MS
    enable_channel $RED
    ) 200>"$LED_LOCK"
}

indicate_success()
{
    check_led_addr
    (
    flock 200
    reset_chip $DRIVER_ADDR
    enable_chip
    set_mode $GREEN $MODE_ONESHOT
    set_brightness $GREEN 0xff
    enable_channel $GREEN
    ) 200>"$LED_LOCK"
}


take_control()
{
    # Do nothing
    return
}

release_control()
{
    (
    flock 200
    if [ -f "$LED_ENABLED" ]; then
        led_on
    else
        led_off
    fi
    ) 200>"$LED_LOCK"
}

stop()
{
    (
    flock 200
    rm "$LED_ENABLED" &>/dev/null
    led_off
    ) 200>"$LED_LOCK"
}

while [ $# -gt 0 ]
do
    case "$1" in
        --start) start ; shift 1 ;;
        --indicate-wait) indicate_wait ; shift 1 ;;
        --indicate-progress) indicate_progress ; shift 1 ;;
        --indicate-error) indicate_error ; shift 1 ;;
        --indicate-success) indicate_success ; shift 1 ;;
        --stop) stop ; shift 1 ;;
        --take-control) take_control ; shift 1 ;;
        --release-control) release_control ; shift 1 ;;
        --fix-addr) fix_addr ; shift 1 ;;
        *)
            echo "$1 is not an option"
            exit 1
            ;;
    esac
done

