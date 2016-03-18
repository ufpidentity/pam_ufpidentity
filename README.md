UFP Identity PAM module
=======================

Overview 
--------
The UFP Identity PAM module provides integration of
the UFP Identity service into your existing user authentication
infrastructure.  PAM is used by Linux/BSD, Solaris and Mac OS X for
user authentication to various services such as SSH, login, and sudo.

Installation
------------
After acquiring the source, and the required dependencies, you can build with:

    make clean
    make
    export LIBDIR=/lib/i386-linux-gnu ; sudo make install

That will install pam_ufpidentity.so into /lib/i386-linux-gnu/security (n.b. change the directory for your multi-arch setup)

Then you need to get credentials for accessing the UFP Identity service. Please read [Getting Started](https://www.ufp.com/identity/integration.html#getting_started)
for an overview. Make sure to have an ASCII representable key for your
private key. And make sure to carefully think about the CN you
use. For a large number of machines you may want a base domain like
example.com, rather than web01.example.com, web02.example.com, etc. On
a Linux machine it is recommended to keep your private keys in
/etc/ssl/private and your certificates in /etc/ssl/certs. You will
also need our truststore somewhere (/etc/ssl/certs is good).

n.b. /etc/ssl/private is a restricted directory. Make sure you
understand and configure appropriate permissions for access to files
in there.

You can test with the check\_user application. Configure the check\_user service with a file called check\_user in /etc/pam.d/
The contents of the file look like:

    # check authorization
    auth    required        pam_ufpidentity.so cert=<certificate_file> passphrase=<keypass>  key=<key_file> truststore=<truststore_file>
    account required        pam_unix.so


Compile the check_user application like:

    gcc -o check_user check_user.c -lpam -lpam_misc

Make sure the check_user application is run with permissions to access the file(s) in /etc/ssl/private

    sudo ./check_user

To configure ssh modifiy the /etc/pam.d/sshd (n.b. do this from a console to ensure that access is not compromised)
Comment out the line that includes common-auth (we no longer want pam_unix to check against /etc/passwd). Under that line add the configuration for UFP Identity

    auth    required        pam_ufpidentity.so cert=<certificate_file> passphrase=<passphrase> key=<key_file> truststore=<truststore_file>

Then some modifications to /etc/ssh/sshd_config are required including:

    ChallengeResponseAuthentication yes
    PasswordAuthentication no 
    UsePAM yes

Restart sshd and try it out. 

Integrate and enroll and we'll send you a free Yubico or you can use
our [iOS OATH app](https://itunes.apple.com/us/app/ufp-identity-oath-token/id794203464?mt=8)
