# Islandora Datastream Dedup 

## Introduction

This module provides a drush command that permits permanently removing older datastream versions, leaving only the most recent. It was developed for a case where multiple versions of very large OBJ datastreams had unintentionally been created and needed to be removed to recover server storage space.

## Requirements

This module requires the following module (and its dependencies):

* [Islandora Solr Search](https://github.com/Islandora/islandora_solr_search)


## Installation

Install as usual, see [this](https://drupal.org/documentation/install/modules-themes/modules-7) for further information.

## Configuration

No configuration is required for this module.


### Usage

This module provides a single drush command: `dedup-datastreams` (alias `ddds`)

Options:

| Option | Info |
|---|---|
| --ds | **[Required]** A datastream identifier, e.g. `--ds=TN` or `--ds=OBJ` |
| --mimetype | **[Required]** The mimetype that the datastream uses. E.g. `--mimetyp="image/jpeg"` |
| --nuke | **[Optional]** This flag must be set to actually perform deletion of the datastreams, e.g. `--nuke`. Otherwise you will only see a report that previews what would have been deleted. |
| --timer | **[Optional]** Output time stats, e.g. `--timer`. |


## Examples:

- `drush ddds -u 1 --ds=OBJ --mimetype="image/jpeg" --nuke --timer`
   Delete duplicate OBJ datastreams whose mimetype is "image/jpeg", and show timing statistics.
- `drush ddds  -u 1 --ds=OBJ --mimetype="image/tiff"`
   Display stats about duplicate OBJ datastreams where the mimetype is "image/tiff". No datastreams will be deleted



## Troubleshooting/Issues

Remember to run this command with the `-u` option. If you do not provide it, deleting datastreams will fail.

Having problems or solved a problem? Check out the Islandora google groups for a solution, or contact the current maintainer.

* [Islandora Group](https://groups.google.com/forum/?hl=en&fromgroups#!forum/islandora)
* [Islandora Dev Group](https://groups.google.com/forum/?hl=en&fromgroups#!forum/islandora-dev)

## Maintainers/Sponsors

Current maintainers:

* [Pat Dunlavey](https://github.com/patdunlavey)

## Development

If you would like to contribute to this module, please check out [CONTRIBUTING.md](CONTRIBUTING.md). In addition, we have helpful [Documentation for Developers](https://github.com/Islandora/islandora/wiki#wiki-documentation-for-developers) info, as well as our [Developers](http://islandora.ca/developers) section on the [Islandora.ca](http://islandora.ca) site.

## License

[GPLv3](http://www.gnu.org/licenses/gpl-3.0.txt)
