# Islandora Datastream Dedup

## Introduction

This module provides a drush command that permits permanently removing older datastream versions, leaving only the most recent.
It was developed for a case where multiple versions of very large OBJ datastreams had unintentionally been created and needed
to be removed to recover server storage space.

## Requirements

This module requires the following module (and its dependencies):

* [Islandora Solr Search](https://github.com/Islandora/islandora_solr_search)


## Installation

Install as usual, see [this](https://drupal.org/documentation/install/modules-themes/modules-7) for further information.

## Configuration

No configuration is required for this module.


### Usage

```$xslt
drush dedup-datastreams --help
Removes older versions of a datastream, preserving the current version. Additional options to remove all datastreams, preview-only, filter by collection, content model.

Examples:
 drush ddds -u 1 --ds=OBJ                Delete duplicate OBJ datastreams whose mimetype is "image/jpeg", and show timing statistics.
 --mimetype="image/jpeg" --op=nuke-dups
 --timer
 drush ddds  -u 1 --ds=OBJ               Display stats about duplicate OBJ datastreams where the mimetype is "image/tiff". No datastreams will be deleted
 --mimetype="image/tiff"

Options:
 --cm                                      Optional: Comma-separated list of content model pids. Restricts datastream deduping to objects that match these content-models.
                                           E.g. `--cm=islandora:newspaperPageCModel,islandora:pageCModel
 --collection-pids                         Optional: Provide one or more collection pids to include children of. Separate multiple pids with commas
 --collection-pids-file                    Optional: Provide path to a file with collection pids to exclude children of. One pid per line.
 --ds                                      A datastream identifier, e.g. "TN" or "OBJ" Required.
 --exclude-pids                            Optional: Negate the collection pids, i.e. exclude children of the pid/pids provided by collection-pids-file and collection-pid.
 --mimetype                                Optional: The mimetype that the datastream uses. E.g. "image/jpeg", or "image/tiff"
 --op                                      Optional: This must be set to "nuke-dups" or "nuke-all" to actually perform deletion of the datastreams. Acceptable values are:
                                           "preview-dups" (report on duplicate datastreams), "preview-all" (report on all copies of the datastream), "nuke-dups" (delete
                                           duplicate datastreams), and "nuke-all" (delete all copies of the datastream). If not provided, defaults to "preview-dups".
 --timer                                   Optional: Output time stats.

Aliases: ddds

```


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
