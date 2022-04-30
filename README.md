# tomsdataonion
How to peel this puzzle. My approach.

Use the Perl program `makeAllCprogs.pl` to compile all C programs.

Issue the command as

`>$ perl makeAllCprogs.pl .`

and all compiled C programs will be saved in your `~/bin/` folder.

To use your `~/bin/` folder add this path to your `PATH` environment variable, usually by adding 

`[ -d "$HOME/bin" ] && PATH="$HOME/bin:$PATH"`

to your `.bashrc` file.
