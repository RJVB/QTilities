from appscript import *
import HRTime
import sys

theMovieDoc = app('QTVOD').open('/Volumes/Win/Video/INRETS/videoTests/P3_Parcours3-S.VOD')

duration = theMovieDoc.duration.get()
theMovieDocsCurrentTime = theMovieDoc.currentTime.get
theMovieDocsNewTime = theMovieDoc.currentTime.set
theMovieDocNextFrame = theMovieDoc.stepForward
currentTime = theMovieDocsCurrentTime()
theMovieDocsNewTime( duration / 2.0 )

print >>sys.stderr, dir(theMovieDoc), theMovieDocsNewTime, type(theMovieDocsNewTime), dir(theMovieDocsNewTime)

print >>sys.stderr, 'Checking maximum playback speed of every frame (first N frames)'
HRTime.tic()
for i in xrange(int(duration+0.5)):
	theMovieDocNextFrame()
t = HRTime.toc()
print >>sys.stderr, '%d frame steps in %g seconds - %gfps' %(i, t, i/t)

print >>sys.stderr, 'Checking maximum "RT" playback speed'
start = HRTime.HRTime()
theMovieDocsNewTime(0)
t = HRTime.HRTime() - start
n = 0
while t < duration:
	theMovieDocsNewTime( t )
	t = HRTime.HRTime() - start
	n += 1
print >>sys.stderr, '%d frames in %g seconds - %gfps' %(n, t, n/t)

dut = int(1.0e6 / theMovieDoc.frameRate.get())
print >>sys.stderr, 'Checking "RT" playback speed using %dus inter-frame intervals' %dut
start = HRTime.HRTime()
theMovieDocsNewTime(0)
t = HRTime.HRTime() - start
n = 0
while t < duration:
	theMovieDocsNewTime( t )
	t = HRTime.HRTime() - start
	n += 1
	HRTime.usleep(dut)
print >>sys.stderr, '%d frames in %g seconds (movie duration is %gs) - %gfps' %(n, t, duration, n/t)
