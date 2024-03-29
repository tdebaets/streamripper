Unofficial fork of [Streamripper](http://streamripper.sourceforge.net/), an application that lets you record streaming MP3 to your hard drive. The source code was obtained both from Streamripper's Git repository and CVS repository at SourceForge, after which the (older) CVS history was merged with the (newer) Git history. That way, all publicly available and historic commits have been preserved.

The last version of Streamripper that was released is 1.64.6, from March 2009. That version has an issue where it's unable to record certain HTTP streams that redirect to a different location (more details further below), which is the main reason why this fork was created. Since development of Streamripper still continued for a while after the last release, the initial idea was to use this fork to create a build based off the latest commits to come up with a fix. However, this quickly proved to be impractical since these builds are too unstable and usually crash a few seconds after recording has been started.

So the alternative route was chosen to create a new branch based on the 1.64.6 code and apply the fix for the redirection issue on top of that. As a result, this newly created branch - [sripper-1_64_6](../../tree/sripper-1_64_6) - should be identical to the released 1.64.6 version, except for the following changes that were applied on top of the original code: (see the `sripper-1_64_6` commit log for more details)

* Fix for Streamripper not picking up HTTP header fields whose name starts with a lowercase character. This is particularly an issue for streams that redirect to a different location by including a lowercase `location:` field in their HTTP response. (backport of commits [751a231](../../commit/751a231) and [f6c9637](../../commit/f6c9637) from the main branch)
* Fix for Streamripper immediately crashing at startup. This is because the pre-compiled version of GLib that is included in the repository was compiled with a different version of the C runtime library (CRT) than the version of the CRT used by Streamripper. As a result, some variables that have been allocated in the main application code can't just be passed to GLib (and vice versa). This was fixed by replacing some GLib library calls in the application code with their CRT counterparts.
* Fix for `SR_ERROR_PARSE_FAILURE` when trying to rip a stream whose URL contains one or more colon characters.
* Not including the host port anymore in the HTTP GET request when it's the default port 80, since some web servers seem to return a `404` response when port 80 is explicitly specified in the request.
* Added Visual Studio 2008 project files, resulting from automatic conversion of the existing VC++ 6 projects, and further modified to make them build successfully.

Obtaining the source code
-------------------------

First make sure that you have a recent version of the [Git client](https://git-scm.com/) (`git`) installed. Then open a Windows command prompt window and run these commands:
```
> git clone https://github.com/tdebaets/streamripper.git streamripper
> cd streamripper
```

To switch to the 1.64.6 branch:

```
> git checkout sripper-1_64_6
```

To keep your repository up-to-date, run the `git pull` command.

Building
--------

On Windows, Visual Studio 2008 is required to build this repository. More recent versions of Visual Studio are currently not supported (although building may still succeed) but this might be addressed in the future.

On other operating systems, it *might* still be possible to build this repository, but this is completely untested, see [INSTALL](INSTALL) for instructions. If this would fail in any way, there is currently no intention to investigate or fix it from my part, but pull requests containing fixes are always welcome.

Installing
----------

On Windows, it's recommended to run the [Streamripper 1.64.6 installer](https://sourceforge.net/projects/streamripper/files/streamripper%20%28current%29/1.64.6/streamripper-windows-installer-1.64.6.exe/download) first, then download the [1.64.6.2 release](../../releases/tag/sripper-1_64_6_2) of this repository, and overwrite the installed binaries with the binaries of the new release.

On other operating systems, the repository needs to be built from scratch to get the appropriate binaries, see the `Building` section above.

License
-------

Streamripper is licensed under the GNU General Public License version 2.0, see [COPYING](COPYING) for details.
