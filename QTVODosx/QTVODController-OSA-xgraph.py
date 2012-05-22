# OSA/python bridge exporting some QTVOD functionality to XGraph
# uses appscript : http://appscript.sourceforge.net/py-appscript/index.html

from appscript import *
import HRTime
import sys
from os import getcwd

QTPlayerName = 'QTVOD'
QTPlayer = app(QTPlayerName)

global theMovieDoc
global duration
global theMovieDocsCurrentTime
global theMovieDocsNewTime

def OpenMovie(fname):
	global theMovieDoc
	global duration
	global theMovieDocsCurrentTime
	global theMovieDocsNewTime
	# NB: fname should be an absolute path. Alternatively, we construct one here
	# if user passes in a relative path (not starting with a '/')
	if( fname[0] != '/' ):
		fname = getcwd() + '/' + fname
	theMovieDoc = QTPlayer.open(fname)
	duration = theMovieDoc.duration.get()
	theMovieDocsCurrentTime = theMovieDoc.currentTime.get
	theMovieDocsNewTime = theMovieDoc.currentTime.set
	return duration

ascanf.ExportVariable("OpenMovie", OpenMovie, replace=True, IDict=True, as_PObj=True, label=sys.argv[0])

def getMovieTime():
	try:
		return theMovieDocsCurrentTime()
	except:
		return -1.0

ascanf.ExportVariable("getMovieTime", getMovieTime, replace=True, IDict=True, as_PObj=True, label=sys.argv[0])

def setMovieTime(t, movieStartDelay=0.0):
	try:
		theMovieDocsNewTime( (t-movieStartDelay) )
	except:
		print >>sys.stderr, 'Failure setting movie time'
	return t

ascanf.ExportVariable("setMovieTime", setMovieTime, replace=True, IDict=True, as_PObj=True, label=sys.argv[0])

print >>sys.stderr, 'Now you can do things in ascanf like OpenMovie["fubar.mov"] and then, in xgraph,'
print >>sys.stderr, '*CROSS_FROMWIN_PROCESS* *SELF*:: setMovieTime[$DATA{0},35], $DATA{1}'
print >>sys.stderr, 'to display the movie frame closest to the time at the mouse cursor, given a movie start delay of 35'
