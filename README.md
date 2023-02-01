# gfw-proxy

## Introduction:

gfw-proxy is a open source software licensed under [GPL-3.0 license](LICENSE.txt) which aims to provide a secure and lightweight method that helps you get rid of the [Great Firewall](https://en.wikipedia.org/wiki/Great_Firewall).

## Inspirations:

We've long firmly held the belief that everyone in the world should have the same equality in accessing human knowledge, as well as the ability to enjoy the [Right to Internet access](https://en.wikipedia.org/wiki/Right_to_Internet_access) (see below). But very unfortunately, if you live in a country where heavy censorship and control are applied everywhere in the internet, you might fail to enjoy your proper rights. Hopefully, we create this software trying our best to give them back to you.

> Right to Internet Access:
> 
> ... all people must be able to access the Internet in order to exercise and enjoy their rights to freedom of expression and opinion and other fundamental human rights, that states have a responsibility to ensure that Internet access is broadly available, and that states may not unreasonably restrict an individual's access to the Internet. 

## How this software work?

You (client) <---(the gfw)---> Proxy Server <-----> Internet

------->---------(can't directly access)-------->--------

All data transferring between you and your proxy server are encrypted (with modern tls1.3 protocol) and look similar to very normal https (http + tls) traffic as long as you send them to port 443 on proxy server.

To receive service from your proxy server, you need to provide a password to help the server identify you from other normal https traffic. If the authorization process fails, you'll be redirect to another real http server and the proxy server acts like a real https server (thus the firewall will be unlikely to block your server just because it looks like a normal https server providing usual services (it doesn't know your password), in another word, you game it!)

## Installation

To get the software working, you need to set up on **both** server side and client side, if didn't yet have any experience in configuring an application, don't worry, the rest of the passage will show you how to do so step by step.

### Server Side Configurations

Introductions:

To get the server working, you'll need...

- a host (server) running linux (I suggest debian for its great stability, but anything else is ok) located somewhere not affected by the firewall (has full internet access), you can buy one from any virtual private server (VPS) provider

- working local http server

- [optional] a domain name (this is not necessary, if you don't have one, don't worry)

- proper x509 certificate used to encrypt and verify data (also no need to worry if you haven't heard about it)

- your great patience and sufficient time (the **most important**)

#### Step 1: get a working http server.

 This is quit easy to do. All you have to do is to install one. For me, I prefer nginx, so I'll show you how to install it here.

Now I suppose you've successfully logged into your vps (via ssh, for example).

1. You need to install nginx using the following command:

```
sudo apt update && sudo apt install -y nginx 
```

if it says something like `apt command not found`, you're probably using centOS/fodera or some other distros using `dnf` as its package manager, in this case, try

```
sudo dnf install nginx
```

if this doesn't work either, you'd better use your search engine to figure out how to install nginx in your linux distribution.

2. start your nginx by issuing

```
sudo systemctl enable --now nginx
```

3. check if your http server is working by

```
sudo lsof -i:80
```

if you get an error message like `lsof command not found`, install it by replace 'nginx' in step 1 with 'lsof', use search engine if you need help.

if everything works well, you'll get some output says nginx is listening on port 80. But if you got nothing, your http server is not running, go back and check what's wrong.

#### Step 2: get a working certificate

Instead of teaching how to get a certificate from a public ca (certificate authority), I'll show you how to create your own one trusted by yourself.

1. get a place to store the certificates
   
   ```
   mkdir ~/certs && cd ~/certs
   ```

2. create ca 
   
   1. create  ca private key
      
      ```
      openssl genrsa -out rootCA.key 4096
      ```
   
   2. create root ca certificate
      
      ```
      openssl req -x509 -new -nodes -key rootCA.key -sha256 -days 3650 -out rootCA.crt
      ```
      
      you need to answer some questions, if you don't know what to fill with, a simple `.` helps.

3. create your certificate
   
   1. get a place to store your certificates and private key
      
      ```
      mkdir ~/certs/my_certs && cd ~/certs/my_certs
      ```
   
   2. create your certificate key
      
      ```
      openssl genrsa -out my_domain.key 4096
      ```
   
   3. create your csr (certificate signing request) file
      
      NOTE: You only need to to choose **ONE** way out of the following 2.
      
      - if you have a subdomain name associate with your vps, use this way (replace <FILL_ME_WITH_YOUR_DNS_NAME> with your real domain name (there're 2 places))
        
        ```
        openssl req -new -sha256 \
            -key my_domain.key \
            -subj "/CN=<FILL_ME_WITH_YOUR_DNS_NAME>" \
            -reqexts v3_req \
            -config <(cat /etc/ssl/openssl.cnf <(printf "\n[v3_req]\nextendedKeyUsage=serverAuth, clientAuth\nsubjectAltName=DNS:<FILL_ME_WITH_YOUR_DNS_NAME>")) \
            -out my_domain.csr
        ```
      
      - if you do **NOT** have a domain name, it's possible to create your certificate directly with the public IP address (replace <FILL_ME_WITH_YOUR_IP_ADDRESS> with your public IP address)
        
        ```
        openssl req -new -sha256 \
            -key my_domain.key \
            -subj "/CN=<FILL_ME_WITH_YOUR_IP_ADDRESS>" \
            -reqexts v3_req \
            -config <(cat /etc/ssl/openssl.cnf <(printf "\n[v3_req]\nextendedKeyUsage=serverAuth, clientAuth\nsubjectAltName=DNS:<FILL_ME_WITH_YOUR_IP_ADDRESS>")) \
            -out my_domain.csr
        ```
   
   4. use ca's private key to sign the request you just created, generating the certificate
      
      NOTE: You only need to to choose **ONE** way out of the following 2, and this way should be the **SAME** as you chose in the previous step
      
      - if you have a subdomain name associate with your vps, use this way (replace  <FILL_ME_WITH_YOUR_DNS_NAME> with your real domain name (there's 1 place))
        
        ```
        openssl x509 -req \
        	-extfile <(printf "[v3_req]\nextendedKeyUsage=serverAuth,clientAuth\nsubjectAltName=DNS:<FILL_ME_WITH_YOUR_DNS_NAME>") 
        	-extensions v3_req -days 360 -in my_domain.csr -CA ../rootCA.crt -CAkey ../rootCA.key \
        	-CAcreateserial -out my_domain.crt -sha256
        ```
      
      - if you use your IP address in the previous step, use the following code (replace <FILL_ME_WITH_YOUR_IP_ADDRESS> with your public IP address)
        
        ```
        openssl x509 -req \
        	-extfile <(printf "[v3_req]\nextendedKeyUsage=serverAuth,clientAuth\nsubjectAltName=DNS:<FILL_ME_WITH_YOUR_IP_ADDRESS>") 
        	-extensions v3_req -days 360 -in my_domain.csr -CA ../rootCA.crt -CAkey ../rootCA.key \
        	-CAcreateserial -out my_domain.crt -sha256
        ```

4. verify your certificates you've created so far
   
   Now you'll need to review what's been done so far. By typing `ls` in the command line, you should see the the directory containing the following stuffs:
   
   - `my_domain.key ` this is your private key, remember path to it, you'll need it later
   
   - `my_domain.csr` this is your csr file, you **don't** need it anymore
   
   - `my_domain.crt` this is your certificate, remember path to it, you'll need it later
   
   #### Step 3: Compile the source code to obtain
