# Guia de configuração 

## EXP 1

### Configuração

Ligação dos cabos:
    - T4 liga à consola
    - T3 liga ao Tux24 S0
    - Tux23 E0 liga à switch
    - Tux24 E0 liga à switch


Primeiramente mudamos para o Tux24, depois temos que abrir o **GTKTerm**, mudar a `Baudrate` do port para 115200 e fazer reset da switch através do seguinte comando:

```
> /system reset-configuration
> y
```

Com isto feito, abrimos o terminal e escrevemos:

```sh
ifconfig eth0 172.16.20.1/24
```

No Tux23 fazemos o mesmo, mas com este comando:

```sh
ifconfig eth0 172.16.20.254/24
```

Podemos fazer ping para confirmar se estão conectados da seguinte forma (no Tux23):

```sh
ping 172.16.20.254
```

Para eliminar as entries na table ARP, para forçar o PC (Tux23, neste caso) a fazer o pedido em broadcast para saber o MAC do PC com quem quer comunicar (Tux24, neste caso), fazemos o seguinte comando:

```sh
arp -d 172.16.20.1
```

### Informação extra

Tux24:
    - IP: **172.16.20.254**
    - MAC: **00:22:64:a6:a4:f1**

Route:
	- Destination: 172.16.20.0
    - Gateway: 0.0.0.0
    - Genmask: 255.255.255.0
    - Flags: U
    - Metric: 0
    - Ref: 0
    - Use: 0
    - Iface: eth0
    
arp -a:
	- IP: 172.16.20.1
	- MAC: 00:21:5a:5a:7d:12

Tux23:
    - IP: 172.16.20.1
    - MAC: 00:21:5a:5a:7d:12

Route:
	- Destination: 172.16.20.0
    - Gateway: 0.0.0.0
    - Genmask: 255.255.255.0
    - Flags: U
    - Metric: 0
    - Ref: 0
    - Use: 0
    - Iface: eth0

arp -a:
	-ip: 172.16.20.254
	-mac: 00:22:64:a6:a4:f1


## EXP 2

### Configuração

Ligação dos cabos:
    - T4 liga à consola
    - T3 liga ao Tux24 S0
    - Tux23 E0 liga à switch
    - Tux24 E0 liga à switch

Primeiramente, vamos para o Tux22 para configurar e registar os endereços de IP e MAC.

```sh
ifconfig eth0 172.16.21.1/24
```

Depois, temos que criar uma bridge que conecta o Tux23 e o Tux24. Para fazer isso temos que fazer o seguinte no terminal:

```sh
interface bridge add name=bridgeY0
interface bridge port remove [find interface=ether3]
interface bridge port remove [find interface=ether4]
interface bridge port add bridge=bridgeY0 interface=ether3
interface bridge port add bridge=bridgeY0 interface=ether4
```

Agora, temos que criar outra bridge que conecta ao Tux22:

```sh
interface bridge add name=bridgeY1
interface bridge port remove [find interface=ether2]
interface bridge port add bridge=bridgeY1 interface=ether2
```

### Informação Extra

Tux22:
    - IP: 172.16.21.1
    - MAC: 00:21:5a:61:2b:72

Route:
    - Destination: 172.16.21.0
    - Gateway: 0.0.0.0
    - Genmask: 255.255.255.0
    - Flags: U
    - Metric: 0
    - Ref: 0
    - Use: 0
    - Iface: eth0