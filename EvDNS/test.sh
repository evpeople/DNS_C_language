#!/bin/bash  
  
for((i=1;i<=100;i++));  
do   
		line=$(sed -n '$(i)p' dnsrelay.txt)
nslookup -port=6801 $line 127.0.0.1
done  
