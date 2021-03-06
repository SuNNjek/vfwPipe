# vfwPipe
Pipes vfw output to an application like ffmpeg via command line

Usage
-----

You can use it with encoders that accept raw video data like ffmpeg. Just select ffmpeg.exe and an output file in the file explorer and pass it with command line parameters like these:
```
-f rawvideo -vcodec rawvideo -s [[width]]x[[height]] -r 30000/1001 -pix_fmt rgb24 -i pipe:0 -c:v libx264 -preset slower -qp 17 "[[output]]"
```

You can also put the placeholders [[width]], [[height]] and [[output]] in the command line and they will be automatically replaced by the corresponding values.

The open new window option is there for programs like ffplay which show video in an extra window.

Build Requirements
------------------

- MSBuild 12 or newer
- Cygwin with git or some other way to start git from your command line; The folder containing git must be in your Path-variable in order for the Version.targets to replace the version number