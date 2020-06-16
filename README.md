#Test of SRT Periodic NAK Tango-2 


**(STILL WIP, DO NOT USE YET!!)**


Prepare the project by ->

```
./prepare_srt.sh
```

Then build using CMake.

##Run the test like this

```
Start the server:

[IP] == Listen interface
[PORT] == Listen port
[INFO_PORT] == JSON port for statistics

./srt_receiver [IP] [PORT] [INFO_PORT]

At the source machine.
Generate MPEG-TS using the included script ./streamts.sh

then start the client

[IP] == Target interface
[PORT] == Target port
[INFO_IP] == JSON interface for statistics
[INFO_PORT] == JSON port for statistics

./srt_sender [IP] [PORT] [INFO_IP] [INFO_PORT]

```

