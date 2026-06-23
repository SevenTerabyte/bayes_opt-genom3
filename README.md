# After writing the .gen file and .idl file, run the commands below for compile:
genom3 skeleton bayes_opt.gen

# Now you have a skeleton, time to implement your functions in the codel folder, finish that before you continue to next step:

# If file are generated and implemented, now
autoreconf -i
mkdir build && cd build
../configure --prefix=$HOME/openrobots --with-templates=pocolibs/server,pocolibs/client/c


# If you modified your code, start from here again
make

# And you should iterate on the make to debug your code for syntax error, after that:
make install

# Now, the Libraries have been installed in location:
/home/lichenjiang/openrobots/lib/genom/pocolibs/plugins

# And remember to apply this path
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$HOME/openrobots/lib/pkgconfig

# Time to run on Terminal 1
h2 init
bayes_opt-pocolibs -b
genomixd &

# Then we open Termial 2, and move into eltclsh and do control via tcl
eltclsh

# Here are setups:
package require genomixd
genomix rpath /home/lichenjiang/openrobots/lib/genom/pocolibs/plugins
genomix load /home/lichenjiang/openrobots/lib/genom/pocolibs/plugins/bayes_opt "" ""
set verbose 0

# And below are commands
set j "{\"lower_bounds\":\[0.0001,0.000001,1.0,1.0,0.0\],\"upper_bounds\":\[0.001,0.0001,10.0,10.0,1.0\],\"max_iterations\":10}"
::genomix::client::bayes_opt send Init $j

# Repeat below:
::genomix::client::bayes_opt send AskNext "{}"
::genomix::client::bayes_opt send UpdateFromMeasure "{}"
::genomix::client::bayes_opt send GetBest "{}"

::genomix::client::bayes_opt send Reset "{}"

# To verify
::genomix::client::bayes_opt read params ""
::genomix::client::bayes_opt read best_result ""
::genomix::client::bayes_opt read status ""

# To input?
::genomix::client::bayes_opt pub measure write "" "{\"x\":0.1,\"y\":0.2,\"z\":0.8,\"vx\":0,\"vy\":0,\"vz\":0,\"valid\":true}"
::genomix::client::bayes_opt pub allow write "" "{\"allow\":true}"

# Afterward: Do following to shutdown, so later you can regenerate again
killall bayes_opt-pocolibs genomixd

