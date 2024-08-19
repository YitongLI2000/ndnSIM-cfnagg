## [1.0.0]- 2024-08-19
### Added
- A new "config.ini" is added to store all relevant input parameters
  - Key parameters are passed into consumer/aggregators correctly, e.g. constraint (C)
  - Network topology will be created automatically based on config information
  - Error handling is enabled, the input parameter's format will be checked before simulation
- Disable auto-generated files (e.g. logs/results) from version control
### Changed
- Rearrange the files regarding logs/results, store them within individual folders
- All result graphs are re-generated, now all individual/combined graphs will be generated
  - The graph's details are tuned, now they're clearer than before
### Fixed
- Fixed the error caused by absolute path
- Fixed the error when importing python3.10 module, now we only need to make sure python3.10
and related libraries are installed successfully. All modules have been imported correctly when execution