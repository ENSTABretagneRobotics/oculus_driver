
import subprocess
import numpy as np
import matplotlib.cm as colormaps

class PingRenderer:

    def __init__(self, height=720, width=-1):
        
        self.height = height
        self.width  = width

        self.masterMode = None
        self.range      = None
        self.aperture   = None
        self.bearings   = None

    def needs_geometry_update(self, msg):
        if self.masterMode is None:
            return True
        if msg.master_mode() != self.masterMode or msg.range() != self.range:
            return True
        return False

    def update_geometry(self, msg):
        if not self.needs_geometry_update(msg):
            return

        self.bearings   = (0.01*np.pi/180.0)*np.array(msg.bearing_data())
        self.masterMode = msg.master_mode()
        self.range      = msg.range()
        self.aperture   = self.bearings[-1] - self.bearings[0]

        pingAspect = 2.0*np.sin(self.bearings[-1])
        if self.width < 0:
            self.width = int(self.height*pingAspect)
        renderAspect = self.width / self.height
        
        x = []
        y = []
        if pingAspect <= renderAspect:
            resolution = self.range / (self.height - 1)
            xmax = 0.5 * self.width * resolution
            x = np.linspace(-xmax,xmax,self.width)
            y = np.linspace(0, self.range, self.height)
        else:
            xmax = self.range*np.sin(self.bearings[-1])
            x = np.linspace(-xmax, xmax, self.width)
            resolution = 2*xmax / (self.width - 1)

            yspan = self.range * pingAspect / renderAspect
            ymin  = -0.5*(yspan - self.range)
            ymax  = ymin + yspan
            y = np.linspace(ymin, ymax, self.height)
        
        X,Y = np.meshgrid(x, y, indexing='xy')

        b = np.arctan2(X, Y)
        r = np.sqrt(X**2 + Y**2)

        self.insideFan = r < self.range
        self.insideFan = np.logical_and(self.insideFan, r > 0)
        self.insideFan = np.logical_and(self.insideFan, np.abs(b) < 0.5*self.aperture)

        self.rangeIndexes = ((msg.range_count()   - 1)
                          * (r[self.insideFan] / self.range)).astype(int)
        self.bearingIndexes = np.interp(
            b[self.insideFan], self.bearings,
            np.linspace(0, len(self.bearings) - 1, len(self.bearings))).astype(int)


    def render(self, msg, img=None):

        self.update_geometry(msg)

        if img is None:
            img = np.zeros([self.height, self.width], dtype=np.float32)

        pingData = np.array(msg.ping_data()).astype(np.float32)
        if msg.has_gains():
            pingData /= np.sqrt(np.array(msg.gains()))[:,np.newaxis]

        img[self.insideFan] = pingData[self.rangeIndexes, self.bearingIndexes]

        return img


# this class needs ffmpeg to be installed
class VideoEncoder:
    
    def __init__(self, videoPath="output.mp4", frameShape=[1280,720], frameRate=10):
        self.path       = videoPath
        self.frameShape = frameShape
        self.colormap   = colormaps.get_cmap('viridis')
        self.imageData  = np.empty([frameShape[1], frameShape[0],4], dtype=np.uint8)

        # self.encoder = sh.Command("ffmpeg")
        # self.arguments = ("-y -f rawvideo -pix_fmt argb -s "
        #                + str(frameShape[0]) + 'x' + str(frameShape[1]) 
        #                + " -r " + str(frameRate)
        #                + " -i - -c:v libx264 -profile:v baseline"
        #                + " -level:v 3 -b:v 2500 -an out_vid.h264").split()
        # for arg in self.arguments:
        #     self.encoder = self.encoder.bake(arg)

        # self.command = ("ffmpeg -y"
        #                + " -f rawvideo -pix_fmt argb"
        #                + " -s " + str(frameShape[0]) + 'x' + str(frameShape[1]) 
        #                + " -r " + str(frameRate)
        #                + " -i - -c:v libx264 -profile:v baseline"
        #                + " -pix_fmt yuv420p"
        #                + " -level:v 3 -b:v 2500 -an out_vid.h264")
        self.command = ("ffmpeg"
                       + " -f rawvideo -pix_fmt rgba"
                       + " -s " + str(frameShape[0]) + 'x' + str(frameShape[1]) 
                       + " -r " + str(frameRate)
                       + " -i - -c:v libx264 -vf fps=10"
                       + " -pix_fmt yuv420p"
                       + " " + self.path)
        print("Encoder command : '" + self.command + "'", flush=True)
        self.encoder = subprocess.Popen(self.command, shell=True,
                                        stdin=subprocess.PIPE)
        self.renderer = PingRenderer(frameShape[1], frameShape[0])
        print("Video encoder ready")

    def add_ping(self, msg):
        img = self.renderer.render(msg)
        m = np.min(img.ravel())
        M = np.max(img.ravel())
        self.imageData[:] = 255*self.colormap((img - m) / (M - m))
        self.add_frame(self.imageData)

    def add_frame(self, img):
        self.encoder.stdin.write(img.tobytes())

    def finish(self):
        self.encoder.terminate()
