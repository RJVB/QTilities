m=figure('position', [0 0 1080 630])
kk=actxcontrol('QTOControl.QTControl.1', [0 0 1080 630])
kk.QuickTimeInitialize()
kk.URL='RB_Parcours2-design.mov'
kk.Movie.SeeAllFrames=1
 
timeScale=kk.Movie.timescale

duration = kk.Movie.duration/timeScale

tic
for t = 0:duration+1
	kk.Movie.time = t*timeScale;
	kk.Movie.Draw();
	pause(0.001);
end
toc
