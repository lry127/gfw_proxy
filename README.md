# gfw-proxy

## Introduction:

gfw-proxy is a open source software licensed under [GPL-3.0 license](LICENSE.txt) which aims at providing a secure and lightweight solution that helps you get rid of the [Great Firewall](https://en.wikipedia.org/wiki/Great_Firewall).

## Inspiration:

We've long firmly held the belief that everyone in the world should have the same equality in accessing human knowledge, as well as the ability to enjoy the [Right to Internet access](https://en.wikipedia.org/wiki/Right_to_Internet_access) (see below). But very unfortunately, if you live in a country where heavy censorship and control are applied everywhere in the internet, you might fail to enjoy your proper rights. Hopefully, we create this software trying our best to give them back to you.

> ... all people must be able to access the Internet in order to exercise and enjoy their rights to freedom of expression and opinion and other fundamental human rights, that states have a responsibility to ensure that Internet access is broadly available, and that states may not unreasonably restrict an individual's access to the Internet. 

## How this software work?

You (client) <---(the gfw)---> Proxy Server <------> Internet

All data transferring between you and your proxy server are encrypted (with modern tls1.3 protocol) and look similar to very normal https (http + tls) traffic as long as you send them to port 443 on proxy server.

To receive service from your proxy server, you need to provide a password to help the server identify you from other normal https traffic. If the authorization process fails, you'll be redirect to another real http server and the proxy server acts like a real https server (thus the firewall will be unlikely to block your server because it looks like a normal https server providing usual services).

## Installation

To get the software working, you need to set up on **both** server side and client side, if didn't yet have any experience in configuring an application, don't worry, the rest of the passage will show you how to do so step by step.

### Server Side Configuration

Introduction:

To get the server working, you'll need...

- a host (server) running linux (I suggest debian for its great stability, but anything else is ok) located somewhere not affected by the firewall (has full internet access), you can buy one from any virtual private server (VPS) provider

- working local http server

- [optional] a domain name (this is not necessary, if you don't have one, don't worry)

- proper x509 certificate used to encrypt and verify data (also no need to worry if you haven't heard about it)

- your great patience and sufficient time (the **most important**)

#### Step 1: get a working http server.

??This is quite easy to do. All you have to do is to install one. For me, I prefer nginx, so I'll show you how to install it here.

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
      
      - if you do **NOT** have a domain name, it's possible to create your certificate directly with the public IP address (replace <FILL_ME_WITH_YOUR_IP_ADDRESS> with your public IP address (there're 2 places))
        
        ```
        openssl req -new -sha256 \
            -key my_domain.key \
            -subj "/CN=<FILL_ME_WITH_YOUR_IP_ADDRESS>" \
            -reqexts v3_req \
            -config <(cat /etc/ssl/openssl.cnf <(printf "\n[v3_req]\nextendedKeyUsage=serverAuth, clientAuth\nsubjectAltName=IP:<FILL_ME_WITH_YOUR_IP_ADDRESS>")) \
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
      
      - if you use your IP address in the previous step, use the following code (replace <FILL_ME_WITH_YOUR_IP_ADDRESS> with your public IP address (there's 1 place))
        
        ```
        openssl x509 -req \
            -extfile <(printf "[v3_req]\nextendedKeyUsage=serverAuth,clientAuth\nsubjectAltName=IP:<FILL_ME_WITH_YOUR_IP_ADDRESS>") 
            -extensions v3_req -days 360 -in my_domain.csr -CA ../rootCA.crt -CAkey ../rootCA.key \
            -CAcreateserial -out my_domain.crt -sha256
        ```

4. verify your certificates you've created so far
   
   Now you'll need to review what's been done so far. By typing `ls` in the command line, you should see the the directory containing the following stuffs:
   
   - `my_domain.key ` this is your private key, remember path to it, you'll need it later
   
   - `my_domain.csr` this is your csr file, you **don't** need it anymore
   
   - `my_domain.crt` this is your certificate, remember path to it, you'll need it later
   
   #### Step 3: Obtain a copy of the server executable file
   
   1. get a place to store the file
      
      ```
      mkdir ~/gfw_proxy && cd ~/gfw_proxy
      ```
   
   2. use wget to get the server executable file
      
      ```
      wget https://github.com/lry127/gfw_proxy/releases/download/v0.1-ga.1/linux-amd64.tar.gz
      ```
   
   3. extract the runnable
      
      ```
      tar xf ./linux-amd64.tar.gz 
      ```
   
   4. verify the software's working
      
      ```
      ./gfw_proxy
      ```
      
      if it works, it should output the following things
      
      ```
      usage: gfw-proxy <path_to_configure_file>
      ```
      
      otherwise, it's not working, use a search engine to see what's wrong
      
      #### Step 4: Write the server side config file
      
      1. Replace the following things in "<>"
         
         ```
         {
             "run_type": "server",
             "password": "<YOUR_PASSWORD>",
             "certificate_path": "<PATH_TO_CERTIFICATE>",
             "private_key": "<PATH_TO_CERTIFICATE_KEY>",
             "listening_address": "0.0.0.0",
             "listening_port": 443,
             "http_service_address": "localhost",
             "http_service_port": 80
         }
         ```
         
         - <YOUR_PASSWORD>: your password
         
         - <PATH_TO_CERTIFICATE>: where your certificate is stored, if you follow the guide strictly, you can see where it's located at by this command (you should copy the output)
           
           ```
           ls $HOME/certs/my_certs/my_domain.crt
           ```
         
         - <PATH_TO_CERTIFICATE_KEY>: where your private key is stored, if you follow the guide strictly, you can see where it's located at by this command (you should copy the output)
           
           ```
           ls $HOME/certs/my_certs/my_domain.key
           ```
      
      2. The sample config file might look like this:
         
         ```
         {
             "run_type": "server",
             "password": "U35RY3KyMnhl",
             "certificate_path": "/root/certs/my_certs/my_domain.crt",
             "private_key": "/root/certs/my_certs/my_domain.key",
             "listening_address": "0.0.0.0",
             "listening_port": 443,
             "http_service_address": "localhost",
             "http_service_port": 80
         }
         ```
      
      3. now write the config to a file called `server.json`
         
         ```
         echo "{
             \"run_type\": \"server\",
            \"password\": \"U35RY3KyMnhl\",
             \"certificate_path\": \"/root/certs/my_certs/my_domain.crt\",
             \"private_key\": \"/root/certs/my_certs/my_domain.key\",
             \"listening_address\": \"0.0.0.0\",
             \"listening_port\": 443,
             \"http_service_address\": \"localhost\",
             \"http_service_port\": 80
         }" > server.json
         ```
      
      4. run the server:
         
         ```
         sudo ./gfw_proxy ./server.json
         ```
         
         if it's working, you might get the following output:
         
         ```
         gfw-proxy start running...
         run type: server
         listening on: 0.0.0.0:443
         using costum certificate: /root/certs/my_certs/my_domain.pem
         using costum private key: /root/certs/my_certs/my_domain.key
         fallback http service is running on: localhost:80
         ```
         
         then you're **DONE**! CONGRATULATIONS!
      
      5. now you'll need to run the server in background and you can safely disconnect from your ssh server. **First press `CTRL+C`**, then type:
         
         ```
         sudo nohup ./gfw_proxy ./server.json &
         ```
      
      6. verify it:
         
         ```
         sudo lsof -i:443
         ```
         
         you'll expect it output something

### Client Side Configuration

Introduction:

To get the client run, all you need are merely a configuration file telling the program about your server and the root CA you just created.

#### Step 1: Obtaining the Root CA file

There are many methods available to you to get the root ca file, if you know how to do so, just get it and skip this step.

- Getting it using scp (supposing you're using root account)
  
  ```
  scp root@<MY_IP_OR_DOMAIN>:~/certs/rootCA.crt .
  ```

- Getting it using https (please first connect to your server)
  
  ```
  cp ~/certs/rootCA.crt /var/www/html
  chmod 644 /var/www/html/rootCA.crt
  ```
  
  now you can get it using your browser:
  
  `https://<MY_IP_OR_DOMAIN>/rootCA.crt`
  
  you may need to ignore warnings your browser complaining about, it's normal.

#### Step 2: Obtaining the executable

1. please download  [here](https://github.com/lry127/gfw_proxy/releases/download/v0.1-ga.1/windows-amd64.zip). (suppose you're using 64 bit Windows operating system)

2. after downloading the .zip file, unzip it. (you need to right click the file and select Extract All...)

3. now we call the folder you extracted from previous step `gfw-proxy`

#### Step 3: Create the config file

1. create a plain text file (.txt), rename it to `client.txt` (or `client.json` if you know how to modify the postfix of a file, it doesn't matter, but you need to remember the name)

2. open the file you just created

3. add the following fields, make sure they match the server configuration file
   
   ```
   {
       "run_type": "client",
       "password": "<YOUR_PASSWORD>",
       "listening_address": "0.0.0.0",
       "listening_port": 10080,
       "server_address": "<YOUR_SERVER_ADDRESS(IP OR DOMAIN NAME)>",
       "server_port": 443,
       "ca_path": "./rootCA.crt"
   }
   ```

4. copy the rootCA.crt file you got from Step 1 to **the same** folder as the executable file and configuration file.

#### Step 4: run the client

1. enter the gfw-proxy folder by double clicking the gfw-proxy

2. click the path (see the picture) ![path](images/path.png)

3. type `cmd` and hit `Enter` on your keyboard

4. type the following command:
   
   ```
   .\gfw_proxy.exe .\client.txt
   ```

5. if it works, it should say:
   
   ```
   gfw-proxy start running...
   run type: client
   listening on: 0.0.0.0:10080
   server is running on: <DOMAIN>:<PORT>
   using ca file to verify server: ./rootCA.crt
   ```

#### Step 5: tell your browser to use the (http) proxy

1. Open your firefox browser (you can download it from the [official site](https://www.mozilla.org/en-US/firefox/download/thanks/))

2. now type `about:preferences#general` and scroll down to the bottom, where you can see `Network Settings`. Click `Settings`.

3. do the same as ![mine](images/firefox_config.png), click ok when you're finished.

4. now you're **DONE!!** Enjoy your work!

## Troubleshooting

You should first search your problems online, if you're unsure, create an issue and I'm willing to help you.

## Thanks

This project uses some of code from the [trojan-gfw/trojan project](https://github.com/trojan-gfw/trojan) but instead provided a http(s) proxy and uses simpler configuration file, as well as providing a much more detailed documentation (guide).
