# vfwPipe
Pipes vfw output to an application like ffmpeg via command line

Usage
=====

You can use it with encoders that accept raw video data like ffmpeg. Just select ffmpeg.exe in the file explorer and pass it with command line like these:
```
-f rawvideo -vcodec rawvideo -s 1280x720 -r 30000/1001 -pix_fmt rgb24 -i pipe:0 -c:v libx264 -preset slower -qp 17 "OUTPUTFILE"
```

The open new window option is there for programs like ffplay which show video in an extra window