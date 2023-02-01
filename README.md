# gfw-proxy: a lightweight solution that helps you bypass the gfw elegantly
## Introduction:
gfw-proxy is a open source software licensed under [GPL-3.0 license](LICENSE.txt) which aims to provide a secure and lightweiht method that helps you get rid of the [Great Firewall](https://en.wikipedia.org/wiki/Great_Firewall).
## Inspirations:
We've long firmly held the belief that everyone in the world should have the same equality in accessing human knowledge, as well as the ability to enjoy the [Right to Internet access](https://en.wikipedia.org/wiki/Right_to_Internet_access)(or see below). But very unfortunately, if you live in a country where heavy censorship and control are applied everywhere in the internet, you might fail to enjoy your proper rights. Hopefully, we create this software trying our best to give them back to you.
> Right to Internet Access:
> ... all people must be able to access the Internet in order to exercise and enjoy their rights to freedom of expression and opinion and other fundamental human rights, that states have a responsibility to ensure that Internet access is broadly available, and that states may not unreasonably restrict an individual's access to the Internet. 

## How this software work?
You (client) <---(the gfw)---> Proxy Server <-----> Internet
------->---------(can't directly access)-------->--------
All data tranferring between you and your proxy server are encrypted (with modern tls1.3 protocol) and look similar to very normal https (http + tls) traffic as long as you send them to port 443 on proxy server.
To receive service from your proxy server, you need to provide a password to help the server identify you from other normal https traffic. If the authorization process fails, you'll be redirect to another real http server and the proxy server acts like a real https server (thus the firewall will be unlikely to block your server just because it looks like a normal https server providing usual services (it doesn't know your password), in another word, you game it!)

## Installation
