Instruction of WPT batch automation command-line tool
Usage: wpt\_batch.py [options](options.md)

Options:

> -h, --help
> show this help message and exit

> -s SERVER, --server=SERVER
> The wpt server URL. The default value is "http://latencylab/" which is a URL to our WebPageTest instance.

> -i URLFILE, --urlfile=URLFILE
> The path of the input URL file (each line of the file should be a http URL like "http://www.google.com/".

> -f OUTPUTDIR, --outputdir=OUTPUTDIR
> The path of the output directory where you want to save the testing results. The default value is a subdirectory called "result" under the current directory. The testing result file is named by url\_wpt-test-id.xml.

> -y CONNECTIVITY, --connectivity=CONNECTIVITY
> Set the connectivity to pre-defined type: DSL, Dial, Fios and custom (case sensitive). When it is custom, you can set the customized connectivity using the options -u/d/l/p below.

> -u BWUP, --bwup=BWUP
> Upload bandwidth (unit: kbps) of the test.The default value is 1500 (i.e., 1.5Mbps).

> -d BWDOWN, --bwdown=BWDOWN
> Download bandwidth (unit: kbps) of the test. The default value is 384.

> -l LATENCY, --latency=LATENCY
> RTT (unit: ms) of the test.

> -p PLR, --plr=PLR
> Packet loss (percentage) rate of the test. The default value is 0.

> -v FVONLY, --fvonly=FVONLY
> First view only. The repeat view is usually is used to test cache. The default value is True.

> -t, --tcpdump
> Enable tcpdump. The default value is False.

> -c SCRIPT, --script=SCRIPT
> Hosted script file. For details, please visit: http://www.webperformancecentral.com/wiki/WebPagetest/Hosted_Scripting.

> -r RUNS, --runs=RUNS
> The number of runs per test. The default value is 9.

> -o LOCATION, --location=LOCATION
> Test location. The default location is from Atlanta data center (and this is the only location available currently.)

> -m MV, --mv=MV
> Video only saved for the median run. The default value is 1.

Usage examples:
> a) ./wpt\_batch.py
> > The execution uses all default configurations. The script reads the ./urls.txt file, submits all of them to http://latencylab WPT server and saves the results (in XML format) in the directory ./result. All the tests are repeated 9 times with DSL connection without packet loss


> b) ./wpt\_batch.py ---urlfile=/foo/bar/urls.txt -runs=3 --connectivity=custom --bwup=384 --bwdown=1500 --latency=100 --plr=1
> > The execution loads the URLs from /foo/bar/urls.txt. All the tests are repeated 3 times with 384kbps upload bandwidth, 1500kbps download bandwidth, 100ms round-trip time, and packet loss rate 1%.


> c) ./wpt\_batch.py --urlfile=/foo/urls.txt --script=/foo/script.txt

> The content of /foo/urls.txt:

> http://www.gmail.com/

> The content of script.txt (NOTICE: the scripting file is tab separated.):
> logData	0

> // bring up the login screen
> navigate	http://www.gmail.com

> logData	1

> // log in
> setValue	name=Email	latency.testing@gmail.com
> setValue	name=Passwd	tester123
> submitForm	id=gaia\_loginform

> The execution will use the login information in the script to enter that gmail account and the result shows the latency of the after-login page gotten loaded.
