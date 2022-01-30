# taskmaster
reversed HTTP backdoor\
anatomy is quite simple:\
the backdoor abuses the registry to keep configurations\
beside the C2 part, the backdoor makes a copy of itself to system32 and uses that\
path when registrying itself as a service that starts automatically\
there's also a functionality to undo the installation, and to change and display the configuration\
the C2 part consists of commands to upload and download files and to run commands on cmd and get results\
note that connection is disguised as HTTP\
TODO: implement CMD command and implement GenURL
repo contains source + C translation + IDA file
