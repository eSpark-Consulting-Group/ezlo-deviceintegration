#!/usr/bin/env python3

# NOTE: How to install python3 and requests module on OpenWRT
# echo "src/gz openwrt_packages http://downloads.openwrt.org/releases/19.07.3/packages/arm_cortex-a7_neon-vfpv4/packages" >> /etc/opkg/distfeeds.conf
# echo "src/gz openwrt_base http://downloads.openwrt.org/releases/19.07.3/packages/arm_cortex-a7_neon-vfpv4/base" >> /etc/opkg/distfeeds.conf
# opkg update
# opkg install python3
# opkg install python3-requests

from http.client import HTTPConnection
HTTPConnection._http_vsn_str = "HTTP/1.1"

import os
import sys
import json
import requests
import multiprocessing
import socket
import logging
import argparse
import time

logger = logging.getLogger("logger")
logger.setLevel(logging.INFO)
log_handler = logging.StreamHandler(sys.stdout)
logger.addHandler(log_handler)

FOSCAM_BUFFER_LIMIT = 960 # NOTE: vistacam 703 can receive at most 960 bytes at a time
ITEM_STATE_TIMEOUT = 10

SIGNAL_TYPE_NOISE = 'noise'
SIGNAL_TYPE_SILENCE = 'silence'

STREAM_CREATE = 'create'
STREAM_DELETE = 'delete'

ARG_IP = '--ip'
ARG_SERVER_PORT = '--server-port'
ARG_STREAM_ID = '--stream-id'
ARG_CONTROL_ID = '--control-id'
ARG_SIGNAL = '--signal'
ARG_FILE = '--file'
ARG_SIZE = '--size'
ARG_DELAY = '--delay'
ARG_COUNT = '--count'
ARG_VERBOSE = '--verbose'
ARG_VERBOSE_SHORT = '-v'

class ArgumentError(RuntimeError):
    pass

class TestError(RuntimeError):
    pass

def getBroadcastsURL(hubIpAddr, httpServerPort):
    return 'http://{}:{}/v1/broadcasts'.format(hubIpAddr, httpServerPort)

def getRequestURL(hubIpAddr, httpServerPort):
    return 'http://{}:{}/v1/request'.format(hubIpAddr, httpServerPort)

def getStreamEndpoint(streamId):
    return '/v1/addons/cameras/stream/audio/{}'.format(streamId)

class ItemUpdatedFilter:
    def __init__(self, itemId):
        self.m_itemId = itemId

    def match(self, msg):
        if not msg.startswith('data:'):
            return None

        msg = msg[len('data:'):]

        try:
            j = json.loads(msg)
        except:
            return None

        if j['id'] != 'ui_broadcast':
            return None

        if j['msg_subclass'] != 'hub.item.updated':
            return None

        result = j['result']

        if result['_id'] != self.m_itemId:
            return None

        return result['value']

class ItemValue:
    def __init__(self, serverURL):
        self.m_serverURL = serverURL

    def set(self, itemId, itemValue):
        msg = {
            "id": "_AUTO_7143",
            "method": "hub.item.value.set",
            "params":{
                "_id": itemId,
                "value": itemValue
            }
        }

        valueSetRequest = json.dumps(msg)
        r = requests.post(self.m_serverURL, data=valueSetRequest)
        if r.status_code != requests.codes.ok:
            raise TestError('Failed to set item ({}) value ({}). Code: {}'.format(\
                    itemId, itemValue, r.status_code))


class BroadcastListener:
    def __init__( self, url, callback ):
        self.m_url = url
        self.m_callback = callback
        self.m_response = None
        self.m_thread = None

    def listenerThread(self):
        while True:
            try:
                self.m_response = requests.get(self.m_url, stream=True)

                if self.m_response.status_code != requests.codes.ok:
                    logger.error('Broadcasts error: Unable to connect to server, code: {}'.format(\
                            self.m_response.status_code))
                    exit(-1)

                if self.m_response.encoding is None:
                    self.m_response.encoding = 'utf-8'

                for line in self.m_response.iter_lines(decode_unicode=True, chunk_size=1):
                    if line:
                        self.m_callback(line)

            except ConnectionError as ex:
                logger.error('Broadcast error: Unable to connect to server: {}'.format(ex))
                exit(-1)

            except Exception as ex:
                logger.warning('Broadcasts error: {}'.format(ex))
                continue

    def start(self):
        if self.m_thread is not None:
            raise TestError("Broadcasts thread is already running")
        self.m_thread = multiprocessing.Process(target=self.listenerThread)
        self.m_thread.start()

    def stop(self):
        if self.m_thread is None:
            raise TestError("Failed to stop broadcasts thread. Thread is not running")

        self.m_thread.terminate()

        if self.m_response is not None:
            self.m_response.close()


class AudioStreamBase:
    def __init__(self, size, generator):
        self.m_size = size
        self.m_generator = generator

    def read(self, size):
        if self.m_size == 0 or size == 0:
            return bytearray()

        if size > self.m_size:
            size = self.m_size
            self.m_size = 0
            return self.m_generator(size)

        self.m_size -= size
        return self.m_generator(size)

class RandomAudioStream(AudioStreamBase):
    def __init__(self, size):
        AudioStreamBase.__init__(self, size=size, generator=lambda size : os.urandom(size) )

class SilentAudioStream(AudioStreamBase):
    def __init__(self, size):
        AudioStreamBase.__init__(self, size=size, generator=lambda size : bytearray([1] * size) )

def sendAudio(address, port, endpoint, stream):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((address, port))
    s.sendall(b'PUT ' + endpoint.encode("ascii") + b' HTTP/1.1\r\n'
              b'Host: ' + address.encode("ascii") + b':' + str(port).encode("ascii") + b'\r\n'
              b'\r\n')

    sended = 0
    while True:
        data = stream.read(FOSCAM_BUFFER_LIMIT)
        s.sendall(data)
        size = len(data)
        sended += size
        if size < FOSCAM_BUFFER_LIMIT:
            break
        time.sleep(0.05)
    return sended


class CallbackBase:
    def __init__(self):
        pass

    def __call__(self, data):
        self.callback_impl(data)

    def callback_impl(self, data):
        pass

class StreamStateCallback(CallbackBase):
    def __init__(self, itemId):
        CallbackBase.__init__(self)
        self.m_openEvent = multiprocessing.Event()
        self.m_closeEvent = multiprocessing.Event()
        self.m_stateFilter = ItemUpdatedFilter(itemId)

    def callback_impl(self, data):
        logger.debug('Callback data: {}'.format(data))

        state = self.m_stateFilter.match(data)
        if state is None:
            return

        logger.debug('Got sream state: {}'.format(state))

        if state == 'opened':
            self.m_openEvent.set()
        elif state == 'closed':
            self.m_closeEvent.set()
        else:
            logger.error('Unknown stream state: {}'.format(state))
            exit(-1)

class Profiling:
    def __init__(self):
        self.index = None
        self.beforeStreamOpened = None
        self.afterStreamOpened = None
        self.beforeStreamSend = None
        self.afterStreamSend = None
        self.beforeStreamClosed = None
        self.afterStreamClosed = None

        self.bytesSend = None
        self.error = None

class ProfilingTable:
    class Row:
        def __init__(self):
            self.index = None
            self.openDuration = None
            self.sendDuration = None
            self.sloseDuration = None
            self.totalDuration = None
            self.bytesSend = None
            self.error = None

    def __init__(self):
        self.m_rowWidth = ProfilingTable.Row()
        self.m_rowWidth.index = 6
        self.m_rowWidth.openDuration = 11
        self.m_rowWidth.sendDuration = 11
        self.m_rowWidth.closeDuration = 12
        self.m_rowWidth.totalDuration = 12
        self.m_rowWidth.bytesSend = 10
        self.m_rowWidth.error = 8

        self.m_totalLength  = self.m_rowWidth.index
        self.m_totalLength += self.m_rowWidth.openDuration
        self.m_totalLength += self.m_rowWidth.sendDuration
        self.m_totalLength += self.m_rowWidth.closeDuration
        self.m_totalLength += self.m_rowWidth.totalDuration
        self.m_totalLength += self.m_rowWidth.bytesSend
        self.m_totalLength += self.m_rowWidth.error

    def printDataRow(self, profiling):
        def isSet(value):
            return value is not None

        def getDurationMs(timeBefore, timeAfter):
            if not isSet(timeBefore):
                return '-'
            if not isSet(timeAfter):
                return '-'
            return (timeAfter - timeBefore) // 10**6 # from ns to ms

        def getValue(value, defaultValue):
            if value is None:
                return defaultValue
            return value

        row = ProfilingTable.Row()
        row.index = profiling.index
        row.openDuration = getDurationMs(profiling.beforeStreamOpened, profiling.afterStreamOpened)
        row.sendDuration = getDurationMs(profiling.beforeStreamSend, profiling.afterStreamSend)
        row.closeDuration = getDurationMs(profiling.beforeStreamClosed, profiling.afterStreamClosed)
        row.totalDuration = getDurationMs(profiling.beforeStreamOpened, profiling.afterStreamClosed)
        row.bytesSend = getValue( profiling.bytesSend, '-')
        row.error = profiling.error

        self.printRow(row)

    def printHeader(self):
        row = ProfilingTable.Row()
        row.index = 'No.'
        row.openDuration = 'open, ms'
        row.sendDuration = 'send, ms'
        row.closeDuration = 'close, ms'
        row.totalDuration = 'total, ms'
        row.bytesSend = 'bytes'
        row.error = 'error'

        self.printLine()
        self.printRow(row)
        self.printLine()

    def printRow(self, row):
        def getElement(value, width):
            return ' {:>{width}}'.format(value, width=width - 1) # 1 is for ' '

        string  = getElement(row.index, self.m_rowWidth.index)
        string += getElement(row.openDuration, self.m_rowWidth.openDuration)
        string += getElement(row.sendDuration, self.m_rowWidth.sendDuration)
        string += getElement(row.closeDuration, self.m_rowWidth.closeDuration)
        string += getElement(row.totalDuration, self.m_rowWidth.totalDuration)
        string += getElement(row.bytesSend, self.m_rowWidth.bytesSend)

        if row.error:
            string += '   {}'.format(row.error)

        print(string)

    def printLine(self):
        print( '-' * (self.m_totalLength) )

def parse_args():
    description  = ''
    description += 'Tool to measure Two-Way audio performance.\n'
    description += 'Utility measures time needed to create/transmit/delete audio stream.\n'
    description += 'Communication with hub occurs via HTTP server.\n'
    description += 'NOTE: \'anonymous access\' and \'insecure access\' must be enabled on the hub.\n'

    epilog = 'TABLE FIELDS\n'

    field = lambda name, descr: '{:>6} - {}\n'.format(name, descr)
    epilog += field('No.', 'Iteration number')
    epilog += field('open', 'Time in milliseconds used to create audio stream')
    epilog += field('send', 'Time in milliseconds used to transmit audio stream')
    epilog += field('close', 'Time in milliseconds used to delete audio stream')
    epilog += field('total', 'Time in milliseconds passed from stream creation to deletion')
    epilog += field('bytes', 'Number of transmited audio bytes')

    epilog += '\n\n'
    epilog += 'EXAMPLES\n'
    epilog += 'Run test with audio file:\n'
    epilog += '  %(prog)s {} <ip> {} <id> {} <id> {} <audio.wav>'.format(\
            ARG_IP, ARG_STREAM_ID, ARG_CONTROL_ID, ARG_FILE)
    epilog += '\n\n'
    epilog += 'Run test with 2048 bytes of generated noise signal:\n'
    epilog += '  %(prog)s {} <ip> {} <id> {} <id> {} {}'.format(\
            ARG_IP, ARG_STREAM_ID, ARG_CONTROL_ID, ARG_SIGNAL, SIGNAL_TYPE_NOISE)
    epilog += '\n\n'
    epilog += 'Run N tests with 1s delay between tests. Use 2048 bytes of generated silent signal:\n'
    epilog += '  %(prog)s {} <ip> {} <id> {} <id> {} {} {} <N> {} 1'.format(\
            ARG_IP, ARG_STREAM_ID, ARG_CONTROL_ID, ARG_SIGNAL, SIGNAL_TYPE_SILENCE,\
            ARG_COUNT, ARG_DELAY)

    parser = argparse.ArgumentParser(
        description=description,
        epilog=epilog,
        formatter_class=argparse.RawTextHelpFormatter)

    parser.add_argument(
        ARG_IP,
        dest='ip',
        help='Hub IP address',
        required=True)

    parser.add_argument(
        ARG_SERVER_PORT,
        dest='port',
        default=17000,
        type=int,
        help='HTTP server on hub',
        required=False)

    parser.add_argument(
        ARG_STREAM_ID,
        dest='streamId',
        metavar='ID',
        help='Stream ID. Same as ID for \'audio_stream\' item',
        required=True)

    parser.add_argument(
        ARG_CONTROL_ID,
        dest='controlId',
        metavar='ID',
        help='ID of \'audio_stream_control\' item',
        required=True)

    group = parser.add_mutually_exclusive_group()

    group.add_argument(
        ARG_SIGNAL,
        dest='signal',
        choices=[SIGNAL_TYPE_NOISE, SIGNAL_TYPE_SILENCE],
        default=SIGNAL_TYPE_SILENCE,
        help='Type of audio signal',
        required=False)

    group.add_argument(
        ARG_FILE,
        dest='file',
        help='Audio file to send. PCM 8KHz 16bit mono WAV file',
        required=False)

    parser.add_argument(
        ARG_SIZE,
        dest='size',
        default=2048,
        type=int,
        help='Size of audio signal in bytes',
        required=False)

    parser.add_argument(
        ARG_DELAY,
        dest='delay',
        default=2,
        type=int,
        help='Delay between test runs',
        required=False)

    parser.add_argument(
        ARG_COUNT,
        dest='count',
        default=1,
        type=int,
        help='Number of test iterations',
        required=False)

    parser.add_argument(
        ARG_VERBOSE_SHORT,
        ARG_VERBOSE,
        dest='verbose',
        action="store_true",
        help='Increase verbosity',
        required=False)

    return parser.parse_args()

def validateArgs(args):
    if args.port <= 0:
        raise ArgumentError("Invalid port range")

    if args.size <= 0:
        raise ArgumentError("Invalid audio stream size")

    if args.delay < 0:
        raise ArgumentError("Invalid delay")

    if args.count < 0:
        raise ArgumentError("Invalid count")

    if args.file and not os.path.exists(args.file):
        raise ArgumentError("Audio file not found")

def sendAudioStream(args):
    streamEndpoint = getStreamEndpoint(args.streamId)
    if args.file:
        with open(args.file, 'rb') as audioFile:
            return sendAudio(args.ip, args.port, streamEndpoint, audioFile)

    audioStream = None
    if args.signal == SIGNAL_TYPE_SILENCE:
        audioStream = SilentAudioStream(args.size)
    else:
        audioStream = RandomAudioStream(args.size)

    return sendAudio(args.ip, args.port, streamEndpoint, audioStream)

def run(args):
    broadcastsURL = getBroadcastsURL(args.ip, args.port)
    requestURL = getRequestURL(args.ip, args.port)

    streamCallback = StreamStateCallback(itemId=args.streamId)
    broadcastListener = BroadcastListener( broadcastsURL, lambda data: streamCallback(data) )
    waitForOpen = lambda : streamCallback.m_openEvent.wait( ITEM_STATE_TIMEOUT )
    waitForClose = lambda : streamCallback.m_closeEvent.wait( ITEM_STATE_TIMEOUT )
    resetOpenEvent = lambda : streamCallback.m_openEvent.clear()
    resetCloseEvent = lambda : streamCallback.m_closeEvent.clear()

    itemValue = ItemValue(requestURL)
    createStream = lambda : itemValue.set(args.controlId, STREAM_CREATE)
    deleteStream = lambda : itemValue.set(args.controlId, STREAM_DELETE)

    profilingTable = ProfilingTable()
    profiling = None

    iterationDelay = 0 if args.count == 1 else args.delay

    broadcastListener.start()

    profilingTable.printHeader()

    for iteration in range(args.count):
        try:
            profiling = Profiling()
            profiling.index = iteration + 1

            logger.debug('Creating audio stream')
            resetOpenEvent()
            profiling.beforeStreamOpened = time.monotonic_ns()
            createStream()

            logger.debug('Waiting audio stream to open')
            if not waitForOpen():
                raise TestError('Failed to create stream: timeout')
            profiling.afterStreamOpened = time.monotonic_ns()
            logger.debug('Audio stream opened')

            logger.debug('Sending audio')
            profiling.beforeStreamSend = time.monotonic_ns()
            profiling.bytesSend = sendAudioStream(args)
            profiling.afterStreamSend = time.monotonic_ns()
            logger.debug('Total bytes send: {}'.format(profiling.bytesSend))

            logger.debug('Closing audio stream')
            resetCloseEvent()
            profiling.beforeStreamClosed = time.monotonic_ns()
            deleteStream()

            logger.debug('Waiting audio stream to close')
            if not waitForClose():
                raise TestError('Failed to delete stream: timeout')
            profiling.afterStreamClosed = time.monotonic_ns()
            logger.debug('Audio stream closed')

        except Exception as ex:
            profiling.error = str(ex)

        profilingTable.printDataRow(profiling)

        time.sleep(iterationDelay)

    logger.debug('Finishing test')
    broadcastListener.stop()

def main():
    args = parse_args()
    if args.verbose:
        logger.setLevel(logging.DEBUG)
    validateArgs(args)
    run(args)

if __name__ == '__main__':
    sys.exit(main())

