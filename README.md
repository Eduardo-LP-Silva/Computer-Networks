# Computer-Networks
This project was developed in the Computer Networks (RCOM) class as its second project. It is divided in two parts, the first one being an FTP application that supports both download and upload to FTP servers, and the second part being a series of scripts to set up a specific network, as well as data collected during the making of it.

## FTP Application
A simple FTP application, allowing for download and upload to FTP servers.

### Usage:
* ./download ftp://[<user>:<password>@]<host>/<url-path>

Examples:

* ./download "ftp://ftp.fe.up.pt/welcome.msg"
* ./download "ftp://speedtest.tele2.net/50MB.zip"
* ./download "ftp://dlpuser@dlptest.com:e73jzTRTNqCN9PYAAjjn@ftp.dlptest.com/curl.txt"

## Computer Network
The computer network set up by these scripts must include 3 separate computers (tux), one Switch and one Cisco Router. Its architecture is as follows:

![image](https://user-images.githubusercontent.com/32617691/52210069-d28d2800-287d-11e9-8d7f-98cfdd41a1aa.png)
