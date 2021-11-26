# TODO List

In no particular order, these are code-found todos I need to open issues about

* DMI lock
  * I can't pretend to know all variants of the hardware, so there may be other hardware
    that uses these GUIDs
    * Lock down the driver so it targets only the Prometheus XVI at this time
    * ~~Change the name to eluk-pxvi-XXX~~
* Build Automation
  * Automated deb generation
  * Maybe automated rpm generation too?
  * Need a flow with build on commit and a manual release. Maybe release on tag?
* Full control panel replication
  * Identify which interfaces need to be replicated and which don't
  * Implement a new module that does the needful to get/set the needful
  * Possibly base on Tuxedo, or start a Rust panel à la System76
  * Possibly upstream the Panel app to S76 for use with Clevoes, not just Quantas
* Repo cleanup permission
  * Upstreaming still uncertain based on the nature of what I'm doing. Does Tux want?
  * If not, seek permission to remove Tuxedo specifics (keep history)
  * Furthermore, make it more "independent"
* Expanded functionality
  * Replicate "metazone" settings
    * Forces a color everywhere
    * `a2 == 0` seems to be "all zones"
    * `a2 == 6` seems to be "all keyboard zones"
