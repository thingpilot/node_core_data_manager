## Node Core Data Manager Release Notes
**v0.5.0** *25/11/2019*

- Update pre-processor directives

**v0.4.0** *20/11/2019*

- Change format of pre-processor directives to work with macros defined in `custom_targets.json`, available from [mbed targets](https://github.com/thingpilot/mbed_targets)

**v0.3.1** *15/11/2019*

- Add support for Thingpilot Wright v1.0.0 
- Add support for Thingpilot Earhart v1.0.0

**v0.3.0** *15/11/2019*

- No longer require board.h due to definition of custom targets

**v0.2.1** *22/10/2019*

- Changed the max table files to 3

**v0.2.0** *10/10/2019*

 - Addition of truncate function
 - Fix issue whereby 0 can be a valid checksum by using parity bit

**v0.1.0** *09/10/2019*

 - Barebones of a lightweight filesystem
 - Functionality to initialise the filesystem
 - Add files (to which you can add, delete, overwrite entries)
 - Files can be deleted by re-initialising the filesystem
