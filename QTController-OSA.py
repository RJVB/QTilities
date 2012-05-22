from appscript import *
import HRTime
import sys

theMovieDoc = app('Quicktime Player 7').open('/Volumes/Win/Video/INRETS/videoTests/LaneSplitting-full.qi2m')

timeScale = float(theMovieDoc.time_scale.get())
duration = theMovieDoc.duration.get()/timeScale
theMovieDocsCurrentTime = theMovieDoc.current_time.get
theMovieDocsNewTime = theMovieDoc.current_time.set
currentTime = theMovieDocsCurrentTime()
theMovieDocsNewTime( duration / 2.0 )

print >>sys.stderr, theMovieDocsNewTime, type(theMovieDocsNewTime), dir(theMovieDocsNewTime)

HRTime.tic()
for i in xrange(int(duration+0.5)):
	theMovieDocsNewTime( i * timeScale )
t = HRTime.toc()
print >>sys.stderr, '%d frame steps in %g seconds - %gfps' %(i, t, t/i)

start = HRTime.HRTime()
theMovieDoc.current_time.set(0)
t = HRTime.HRTime() - start
n = 0
while t < duration:
	theMovieDocsNewTime( t * timeScale )
	t = HRTime.HRTime() - start
	n += 1
print >>sys.stderr, '%d frame steps in %g seconds' %(n, t)
