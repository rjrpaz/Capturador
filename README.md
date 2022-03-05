# Capturador

Program that extracts billing information from PBX using serial port.

## Context

I developed a *centralized phone billing system* around 2000 for the state administration where I live (Córdoba, Argentina). More info [here](https://porsiserompeeldisco.blogspot.com/2014/01/sistema-centralizado-de-tarifacion.html) (in spanish).

The local government had a lot of dependencies, each one with a specific PBX. In most cases this were different PBX and each one had its own logging format.

Sometimes billing was not even enabled and documentation was usually really hard to find. I don't know is this is common in all places, but here, PBX vendors don't move a finger for free and this includes -off course-, enable billing and providing manual (thanks god for *asterisk*).

Anyways ... part of the centralized billing system included the set of commands that read the serial port, using very old COMPAQ computers that were recovered from the trash. This hardware was discarded for a Cell phone company (called CTI) and "donated" to the Government (these computers were part of a cell base "Class I" that were replaced with new generation cell bases).

The computers were fine to run a linux system, so I installed Slackware + SSH and added the capturer on each case. After each billing record was received from the PBX, the useful information was extracted and uploaded in a centralized database using a custom TCP/IP service.

## Included programs

All programs named as *cap** are capturers from PBX billing system

All programs named as *dbup** uploads the billing information to the custom TCP service. In case the communication failed, the info is stored in a file in a local spooler, waiting to be uploader later.

All programs named as *spooler** is a command that traverses the spooler and upload the missing information. It deletes the file in the spooler if the info was uploaded correctly.

*Centrales* includes info about some aspecto of the billing log once I enabled it in some PBXs.

