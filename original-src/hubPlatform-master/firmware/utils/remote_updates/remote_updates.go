package main

import (
    "fmt"
    "strings"
    "log"
    "os"
    "os/exec"
    "path/filepath"
    "encoding/json"
    "crypto/aes"
    "crypto/cipher"
    "time"
    "errors"
    "flag"
)

// IV: "6d54c2d6111da0201a6fd411912926c8"
var SecretsIV = []byte{
    0x6d, 0x54, 0xc2, 0xd6, 0x11, 0x1d, 0xa0, 0x20,
    0x1a, 0x6f, 0xd4, 0x11, 0x91, 0x29, 0x26, 0xc8 }

// Key: "d914583441a045dff841aa424515f568a0c74993b48730952f3090dbb12a70c6"
var SecretsKey = []byte{
    0xd9, 0x14, 0x58, 0x34, 0x41, 0xa0, 0x45, 0xdf,
    0xf8, 0x41, 0xaa, 0x42, 0x45, 0x15, 0xf5, 0x68,
    0xa0, 0xc7, 0x49, 0x93, 0xb4, 0x87, 0x30, 0x95,
    0x2f, 0x30, 0x90, 0xdb, 0xb1, 0x2a, 0x70, 0xc6 }

var WithDebug = false
func debug( msg string ) {
    if WithDebug {
        log.Println( "[DEBUG] " + msg )
    }
}

func getBinaryPath() string {
    executable, err := os.Executable()
    if err != nil {
        panic(err)
    }
    return filepath.Dir(executable)
}

func isFileExists( path string ) bool {
    if _, err := os.Stat(path); errors.Is(err, os.ErrNotExist) {
        return false
    }
    return true
}

type Settings struct {
    RemoteIp string
    RemoteUser string
    FirmwareFile string
    ControllersFile string
    Debug bool
}

func readSettings() (Settings, error) {
    settings := Settings{}

    remoteIpPtr := flag.String("remote-ip", "192.168.123.2", "Remote IP")
    remoteUserPtr := flag.String("remote-user", "root", "Remote username")
    firmwareFilePtr := flag.String("firmware", "", "Firmware file")
    controllersFilePtr := flag.String("controllers", "", "Controllers file")
    debugFlagPtr := flag.Bool("debug", false, "Debug info")
    flag.Parse()

    settings.RemoteIp = *remoteIpPtr
    settings.RemoteUser = *remoteUserPtr
    settings.FirmwareFile = *firmwareFilePtr
    settings.ControllersFile = *controllersFilePtr
    settings.Debug = *debugFlagPtr

    if 0 == len(settings.FirmwareFile) {
        return settings, errors.New("Missing argument: --firmware")
    }

    if 0 == len(settings.ControllersFile) {
        return settings, errors.New("Missing argument: --controllers")
    }

    fmt.Printf("Settings.RemoteIp: %s\n", settings.RemoteIp)
    fmt.Printf("Settings.RemoteUser: %s\n", settings.RemoteUser)
    fmt.Printf("Settings.FirmwareFile: %s\n", settings.FirmwareFile)
    fmt.Printf("Settings.ControllersFile: %s\n", settings.ControllersFile)
    fmt.Printf("Settings.Debug: %t\n", settings.Debug)

    return settings, nil
}

type ControllerData struct {
    Mac        string    `json:"mac"`
    Password   string    `json:"password"`
}

func parseControllerData( jsonData []byte ) (map[string]ControllerData, error) {
    data := make( map[string]ControllerData )

    var elements []ControllerData

    err := json.Unmarshal([]byte(jsonData), &elements)
    if err != nil {
        return data, err
    }

    for _, element := range elements {
        element.Mac = strings.ToLower(element.Mac)
        data[element.Mac] = element
    }

    return data, nil
}

func decryptAES256CBC(ciphertext []byte, key []byte, iv []byte) ([]byte, error) {
    block, err := aes.NewCipher(key)
    if err != nil {
        return nil, err
    }

    if len(ciphertext)%aes.BlockSize != 0 {
        return nil, fmt.Errorf("ciphertext is not a multiple of the block size")
    }

    mode := cipher.NewCBCDecrypter(block, iv)

    plaintext := make([]byte, len(ciphertext))
    mode.CryptBlocks(plaintext, ciphertext)

    // Remove PKCS#7 Padding
    // Padding is always added to the end (even if data was aligned)
    // and their values are the length of the padding
    length := len(plaintext)
    paddingSize := int(plaintext[length-1])
    plaintext = plaintext[:length-paddingSize]

    return plaintext, nil
}

func readEncryptedControllersData( path string, key []byte, iv []byte ) (map[string]ControllerData, error) {
    log.Printf("Reading controllers data from file: %s\n", path)

    data := make( map[string]ControllerData )

    cyphertext, err := os.ReadFile(path)
    if err != nil {
        log.Printf("Failed to read controllers data: %v\n", err)
        return data, err
    }

    plaintext, err := decryptAES256CBC( cyphertext, key, iv )
    if err != nil {
        log.Printf("Failed to decrypt controllers data: %v\n", err)
        return data, err
    }

    data, err = parseControllerData( plaintext )
    if err != nil {
        log.Printf("Failed to parse controllers data: %v\n", err)
        return data, err
    }
    return data, nil
}

func getMacAddress( ip string ) string {
    // NOTE: One cannot use 'arp -an' on the controller, since it is using busybox
    // and does not have decicated 'arp' binary. Read /proc/net/arp instead

    script := fmt.Sprintf("cat /proc/net/arp | grep -w '%s' | awk '{print $4}'", ip)
    cmd := exec.Command( "/bin/sh", "-c", script )
    output, err := cmd.Output()
    if err != nil {
        log.Printf( "Unable to read MAC address: %v, output: %s", err, string(output) )
        return ""
    }

    return strings.ToLower(strings.TrimSpace(string(output)))
}

func isHostPresent( ip string, pingCount int ) bool {
    cmd := exec.Command( "ping", "-c", fmt.Sprintf("%d", pingCount), "-W", "2", ip )
    _, err := cmd.Output()
    if err != nil {
        return false
    }
    return true
}

func waitForConnect( ip string, timeout time.Duration ) bool {
    beg := time.Now()

    for {
        if isHostPresent( ip, 1 ) {
            return true
        }

        now := time.Now()
        difference := now.Sub(beg)
        if difference > timeout {
            break
        }
    }
    return false
}

func waitForDisconnect( ip string, timeout time.Duration ) bool {
    beg := time.Now()

    for {
        if !isHostPresent( ip, 3 ) {
            return true
        }

        now := time.Now()
        difference := now.Sub(beg)
        if difference > timeout {
            break
        }
    }
    return false
}

func waitForPermanentDisconnect( ip string, disconnectWindow time.Duration, timeout time.Duration ) bool {
    beg := time.Now()

    disconnectBegTime := time.Now()
    disconnectEndTime := time.Now()
    disconnectedBefore := false

    for {
        if !isHostPresent( ip, 3 ) {
            if disconnectedBefore {
                disconnectEndTime = time.Now()
                if disconnectEndTime.Sub(disconnectBegTime) > disconnectWindow {
                    return true
                }
            } else {
                disconnectBegTime = time.Now()
            }
            disconnectedBefore = true
        } else {
            disconnectedBefore = false
        }

        now := time.Now()
        difference := now.Sub(beg)
        if difference > timeout {
            break
        }
    }
    return false
}

type RemoteHost struct {
    Ip string
    User string
    Password string
}

func runRemoteCommand( remoteHost RemoteHost, command string ) (string, error) {
    debug( fmt.Sprintf( "Trying to execute command on remote host: %s, user: %s, cmd: %s\n", remoteHost.Ip, remoteHost.User, command ) )

    cmd := exec.Command( "sshpass", "-p", remoteHost.Password,
        "ssh",
        "-o", "StrictHostKeyChecking=no",
        "-o", "UserKnownHostsFile=/dev/null",
        "-o", "LogLevel=ERROR",
        remoteHost.User + "@" + remoteHost.Ip,
        command )

    output, err := cmd.CombinedOutput()
    return strings.TrimRight(string(output), "\r\n"), err
}

func copyFileToRemote( remoteHost RemoteHost, localFile string, remoteFile string ) (string, error) {
    debug( fmt.Sprintf( "Trying to copy file to remote host: %s, LocalFile: %s, RemoteFile: %s\n", remoteHost.Ip, localFile, remoteFile ) )
    remotePath := fmt.Sprintf( "%s@%s:%s", remoteHost.User, remoteHost.Ip, remoteFile )

    cmd := exec.Command( "sshpass", "-p", remoteHost.Password,
        "scp",
        "-o", "StrictHostKeyChecking=no",
        "-o", "UserKnownHostsFile=/dev/null",
        localFile,
        remotePath )

    output, err := cmd.CombinedOutput()
    return strings.TrimRight(string(output), "\r\n"), err
}


type Led struct {
    LedCmdPath string
}

func createLed() Led {
    led := Led{ LedCmdPath: getBinaryPath() + "/led_cmd.sh" }

    if !isFileExists(led.LedCmdPath) {
        log.Fatalf( "Failed to locate Led script file: %s\n", led.LedCmdPath )
    }
    return led
}

func (led *Led) IndicateWait() {
    exec.Command( led.LedCmdPath, "--indicate-wait" ).Output()
}

func (led *Led) IndicateProgress() {
    exec.Command( led.LedCmdPath, "--indicate-progress" ).Output()
}

func (led *Led) IndicateError() {
    exec.Command( led.LedCmdPath, "--indicate-error" ).Output()
}

func (led *Led) IndicateSuccess() {
    exec.Command( led.LedCmdPath, "--indicate-success" ).Output()
}

const (
    ServiceStateUnknown int = 0
    ServiceStateWaiting int = 1
    ServiceStateUpdating int = 2
    ServiceStateFailed int = 3
    ServiceStateFinished int = 4
)

type Service struct {
    m_led Led
    m_settings Settings
    m_controllersData map[string]ControllerData
    m_firmwareImageChecksum string
    m_remoteFirmwareFile string
    m_state int
}

func (s *Service) SetUp(led Led, settings Settings) {
    s.m_state = ServiceStateUnknown
    s.m_led = led
    s.m_settings = settings
    s.m_remoteFirmwareFile = "/tmp/sysupgrade.img"

    if !isFileExists( s.m_settings.FirmwareFile ) {
        s.SetState(ServiceStateFailed)
        log.Fatalf("Firmware file does not exists: %s", s.m_settings.FirmwareFile)
    }

    if !isFileExists( s.m_settings.ControllersFile ) {
        s.SetState(ServiceStateFailed)
        log.Fatalf("Controllers file does not exists: %s", s.m_settings.ControllersFile)
    }

    s.InitControllersData()
    s.InitImageChecksum()
}

func (s *Service) InitControllersData() {
    data, err := readEncryptedControllersData( s.m_settings.ControllersFile, SecretsKey, SecretsIV )
    if err != nil {
        s.SetState(ServiceStateFailed)
        log.Fatal(err)
    }
    s.m_controllersData = data
    log.Printf("Controllers data total records: %d\n", len(s.m_controllersData))
    //for key, data := range s.m_controllersData {
    //    fmt.Printf("Decrypted data: Key: %s, MAC: %s, Password: %s\n", key, data.Mac, data.Password)
    //}
}

func (s *Service) InitImageChecksum() {
    log.Printf( "Calculating firmware image hash: %s\n", s.m_settings.FirmwareFile )
    cmd := exec.Command( "/bin/sh", "-c", "md5sum " + s.m_settings.FirmwareFile + " | awk '{print $1}'" )
    output, err := cmd.Output()
    if err != nil {
        s.SetState(ServiceStateFailed)
        log.Fatalf( "Unable to calculate firmware image checksum: %v", err)
    }

    s.m_firmwareImageChecksum = strings.TrimRight(string(output), "\r\n")
    log.Printf( "Firmware image hash: %s\n", s.m_firmwareImageChecksum )
}

func (s *Service) SetState( state int ) {
    if state == s.m_state {
        return
    }

    s.m_state = state

    if state == ServiceStateWaiting {
        log.Printf( "Change service state to: %s\n", "Waiting" )
        s.m_led.IndicateWait()
    } else if state == ServiceStateUpdating {
        log.Printf( "Change service state to: %s\n", "Updating" )
        s.m_led.IndicateProgress()
    } else if state == ServiceStateFailed {
        log.Printf( "Change service state to: %s\n", "Failed" )
        s.m_led.IndicateError()
    } else if state == ServiceStateFinished {
        log.Printf( "Change service state to: %s\n", "Finished" )
        s.m_led.IndicateSuccess()
    } else if state == ServiceStateUnknown {
        log.Printf( "Change service state to: %s\n", "Unknown" )
    } else {
        log.Printf( "Unsupported state recieved: %d\n", state )
        s.m_state = ServiceStateUnknown
    }
}

func (s *Service) Run() {
    ipAddress := s.m_settings.RemoteIp
    invalidMac := ""

    s.SetState( ServiceStateWaiting )

    for {
        if !waitForConnect( ipAddress, 5 * time.Second ) {
            debug( fmt.Sprintf( "Not connected: timeout: %s\n", ipAddress ) )
            time.Sleep( 5 * time.Second )
            s.SetState( ServiceStateWaiting )
            continue
        }

        macAddress := getMacAddress( ipAddress )
        if len(macAddress) == 0 || macAddress == invalidMac {
            debug( fmt.Sprintf( "Not supported MAC: %s\n", macAddress ) )
            time.Sleep( 5 * time.Second )
            continue
        }

        log.Printf( "Host detected, IP: %s, MAC: %s\n", ipAddress, macAddress )

        record, found := s.m_controllersData[macAddress]
        if !found {
            log.Printf( "Controller not found in controllers file: MAC: %s\n", macAddress )
            invalidMac = macAddress
            s.SetState( ServiceStateFailed )
            continue
        }

        s.SetState( ServiceStateUpdating )
        remoteHost := RemoteHost{ Ip: ipAddress, User: s.m_settings.RemoteUser, Password: record.Password }
        if s.RunRecovery( remoteHost ) {
            s.SetState( ServiceStateFinished )
        } else {
            s.SetState( ServiceStateFailed )
        }

        log.Printf( "Waiting controller to disconnect, IP: %s\n", ipAddress )
        for false == waitForPermanentDisconnect( ipAddress, 10 * time.Second, 30 * time.Second) {
        }

        log.Printf( "Controller disconnected\n" )
        s.SetState( ServiceStateWaiting )
    }
}

func (s *Service) RunRecovery( remoteHost RemoteHost ) bool {
    log.Printf( "Starring recovery action on host: %s\n", remoteHost.Ip )

    if !s.RunGetSerial( remoteHost ) {
        return false
    }

    if !s.RunGetFirmwareVersion( remoteHost, "current" ) {
        return false
    }

    //if !s.RunWaitForFirmwareInit( 15 * time.Second ) {
    //    return false
    //}

    //if !s.RunStopFirmware( remoteHost ) {
    //    return false
    //}

    if !s.RunUploadFirmware( remoteHost ) {
        return false
    }

    if !s.RunVerifyUploadedFirmwareHash( remoteHost ) {
        return false
    }

    if !s.RunSysupgrade( remoteHost ) {
        return false
    }

    if !s.RunWaitForSysupgrade( remoteHost ) {
        return false
    }

    if !s.RunWaitForFirmwareInit( 30 * time.Second ) {
        return false
    }

    if !s.RunGetFirmwareVersion( remoteHost, "new" ) {
        return false
    }

    if !s.RunSync( remoteHost ) {
        return false
    }

    return true;
}

func (s *Service) RunGetSerial( remoteHost RemoteHost ) bool {
    output, err := runRemoteCommand( remoteHost, "/opt/firmware/bin/at88util -s" )
    if err != nil {
        log.Printf( "Failed to get controller serial, error: %v, output: %s\n", err, output )
        return false
    }
    log.Printf( "Controller serial: %s\n", output )
    return true
}

func (s *Service) RunGetFirmwareVersion( remoteHost RemoteHost, versionType string ) bool {
    output, err := runRemoteCommand( remoteHost, "/opt/firmware/bin/at88util -v" )
    if err != nil {
        log.Printf( "Failed to get controller %s FW version, error: %v, output: %s\n", versionType, err, output )
        return false
    }
    log.Printf( "Controller %s FW version: %s\n", versionType, output )
    return true
}

func (s *Service) RunStopFirmware( remoteHost RemoteHost ) bool {
    // NOTE: stop firmware is needed to prevent remote
    // network manager from resetting the network
    log.Printf( "Stopping remote firmware...\n" )
    output, err := runRemoteCommand( remoteHost, "/etc/init.d/firmware stop" )
    if err != nil {
        log.Printf( "Failed to stop remote firmware, error: %v, output: %s\n", err, output )
        return false
    }
    log.Printf( "Remote firmware stopped\n" )
    return true
}

func (s *Service) RunUploadFirmware( remoteHost RemoteHost ) bool {
    log.Printf( "Uploading firmware...\n" )
    output, err := copyFileToRemote( remoteHost, s.m_settings.FirmwareFile, s.m_remoteFirmwareFile )
    if err != nil {
        log.Printf( "Failed to upload firmware, error: %v, output: %s\n", err, output )
        return false
    }
    log.Printf( "Firmware uploaded\n" )
    return true
}

func (s *Service) RunVerifyUploadedFirmwareHash( remoteHost RemoteHost ) bool {
    log.Printf( "Calculating uploaded firmware hash...\n" )
    output, err := runRemoteCommand( remoteHost, "md5sum " + s.m_remoteFirmwareFile + " | awk '{print $1}'" )
    if err != nil {
        log.Printf( "Failed to get uploaded firmware hash, error: %v, output: %s\n", err, output )
        return false
    }

    if output != s.m_firmwareImageChecksum {
        log.Printf( "Hashes mismatch, local: %s vs remote: %s\n", s.m_firmwareImageChecksum, output )
        return false
    }
    log.Printf( "Hashes matched: %s\n", output )
    return true
}

func (s *Service) RunSysupgrade( remoteHost RemoteHost ) bool {
    // NOTE: sysupgrade -n will drop any previous configs if they were broken
    log.Printf( "Starting sysupgrade\n" )

    command := fmt.Sprintf( "( sleep 3; ifdown lan; /opt/firmware/bin/ha-ledcmd --start firmwareUpdate ; sysupgrade %s ) > /dev/null 2>&1 &", s.m_remoteFirmwareFile )
    output, err := runRemoteCommand( remoteHost, command )
    if err != nil {
        log.Printf( "Failed start sysupgrade, error: %v, output: %s\n", err, output )
        return false
    }
    log.Printf( "Sysupgrade in progress...\n" )
    return true
}

func (s *Service) RunWaitForSysupgrade( remoteHost RemoteHost ) bool {
    log.Printf( "Expecting sysupgrade and reboot sequence...\n" )
    log.Printf( "Waiting controller to disconnect...\n" )

    if !waitForPermanentDisconnect( remoteHost.Ip, 10 * time.Second, 120 * time.Second) {
        log.Printf( "Controler is unable to disconnect\n" )
        return false
    }
    log.Printf( "Controller disconnected\n" )

    log.Printf( "Waiting controller to reconnect...\n" )
    if !waitForConnect( remoteHost.Ip, 180 * time.Second ) {
        log.Printf( "Controler is unable to reconnect\n" )
        return false
    }

    log.Printf( "Controller reconnected\n" )
    return true
}

func (s *Service) RunWaitForFirmwareInit( duration time.Duration ) bool {
    log.Printf( "Waiting %f seconds to let firmware finish initialization...\n", duration.Seconds() )
    time.Sleep( duration )
    return true
}

func (s *Service) RunSync( remoteHost RemoteHost ) bool {
    output, err := runRemoteCommand( remoteHost, "sync" )
    if err != nil {
        log.Printf( "Failed run sync command, error: %v, output: %s\n", err, output )
        return false
    }
    return true
}

func main() {
    led := createLed()

    settings, err := readSettings()
    if err != nil {
        led.IndicateError()
        log.Fatalf("Unable to read settings, error: %v\n", err)
    }
    WithDebug = settings.Debug

    service := Service{}
    service.SetUp( led, settings )
    service.Run()
}

