# ZNC Web Log Module

The ZNC Web Log module allows for the viewing and downloading of log files created by the [Log Module](https://wiki.znc.in/Log). 
The module is based on an earlier python version of the module that can be found [here](https://github.com/LorenzoAncora/znc-weblog)

### Installation
* Download this repository (wget https://github.com/RussellB28/znc-weblog/archive/master.zip) into the source directory of your ZNC Installation
* Unzip master.zip (using unzip master.zip)
* From the source directory, run ```./znc-buildmod modules/weblog.cpp modules/weblog.so```
* Run ```make install``` **or** copy the compiled module to the directory your znc is running in (cp modules/weblog.so /path/to/znc/lib/znc/weblog.so)
* Run ```make install``` **or** Copy the web templates to the directory your znc is running on (cp -R modules/data/weblog /path/to/znc/share/znc/modules)

Alternatively you can download the master.zip elsewhere and copy the files into the source directory yourself. The directory/file structure is as follows
```
ZNC Source Directory
└── modules
    ├── data
    │   └── weblog
    │       └── tmpl
    │           ├── Breadcrumbs.tmpl
    │           ├── download.tmpl
    │           ├── index.tmpl
    │           ├── log.tmpl
    │           ├── raw.tmpl
    │           └── Scope.tmpl
    └── weblog.cpp
```

Once you have done this you need to load the module either using webadmin (Global Settings) or using 
```/msg *status loadmod --type=global weblog```

You should now be able to access the log viewer via the webadmin menu

### Info regarding log sizes ###
While testing this module, we discovered that although logs can be viewed, there is an issue with file sizes over 120~ MB (Megabytes) in size. ZNC's log module creates a new log for channels and windows when the day changes and thus unless you are on a network/channel that is full of spam, it is relatively unlikely for a log to ever reach this size in such a short space of time. From our tests, we've concluded that ZNC will be able to load logs up to 120 MB in size but may take a small amount of time to read the file/prepare the download. **However...** if a log file is over the size of 120MB, there is a risk that your ZNC instance will hang. You have been warned!

### Bugs/Feature suggestions ###
If you've found a bug or have any suggestions on how this module can be improved further, throw us a line [here](https://github.com/RussellB28/znc-weblog/issues) and we'll take a look.
