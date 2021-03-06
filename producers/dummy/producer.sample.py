#!/usr/bin/python
# Karl Palsson, 2010
# A mini producer that can generate some fake data for testing with.

from stompy.simple import Client
import sys, os, time, socket, random
sys.path.append(os.path.join(sys.path[0], "../../common"))
import kpacket
import logging
import jsonpickle
logging.basicConfig(level=logging.DEBUG, format="%(asctime)s %(levelname)s %(name)s - %(message)s")
log = logging.getLogger("main")

stomp = Client(host='egri')

def runMain():
    clientid = "karlnet_fake_producer@%s/%d" % (socket.gethostname(), os.getpid())
    stomp.connect(clientid=clientid)
    
    while True:
        fakeNodeId = 0xbabe
        s1 = kpacket.Sensor(type=36, raw=1234, value=random.randint(0,100))
        s2 = kpacket.Sensor(type=69, raw=4321, value=random.randint(40,80))
        kp = kpacket.human_packet(fakeNodeId, [s1, s2])
        log.info("Producing the dummy data: %s", kp)
        stomp.put(jsonpickle.encode(kp), destination="/topic/karlnet.%d" % fakeNodeId)
        time.sleep(5)


if __name__ == "__main__":
    try:
        runMain()
    except KeyboardInterrupt:
        print "got a keyboard interrupt"
        log.info("QUIT - quitting due to keyboard interrupt")
        stomp.disconnect()
        raise SystemExit

