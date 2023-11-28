# Guia de configuração 

## EXP 1

### Configuração

Ligação dos cabos:

    - T4 liga à consola
    - T3 liga ao Tux24 S0
    - Tux23 E0 liga à switch
    - Tux24 E0 liga à switch


Primeiramente mudamos para o Tux24, depois temos que abrir o **GTKTerm**, mudar a `Baudrate` do port para 115200 e fazer reset da switch através do seguinte comando:

```shell
> /system reset-configuration
> y
```

Com isto feito, abrimos o terminal e escrevemos:

```shell
ifconfig eth0 172.16.20.254/24
```

No Tux23 fazemos o mesmo, mas com este comando:

```shell
ifconfig eth0 172.16.20.1/24
```

Podemos fazer ping para confirmar se estão conectados da seguinte forma (no Tux23):

```shell
ping 172.16.20.254
```

Para eliminar as entries na table ARP, para forçar o PC (Tux23, neste caso) a fazer o pedido em broadcast para saber o MAC do PC com quem quer comunicar (Tux24, neste caso), fazemos o seguinte comando:

```shell
arp -d 172.16.20.1
```

### Informação extra

**Tux24**:

    - IP: 172.16.20.254
    - MAC: 00:22:64:a6:a4:f1

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

**Tux23**:

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

```shell
ifconfig eth0 172.16.21.1/24
```

Depois, temos que criar uma bridge que conecta o Tux23 e o Tux24. Para fazer isso temos que fazer o seguinte no **GTKTerm**:

```shell
interface bridge add name=bridgeY0
interface bridge port remove [find interface=ether3]
interface bridge port remove [find interface=ether4]
interface bridge port add bridge=bridgeY0 interface=ether3
interface bridge port add bridge=bridgeY0 interface=ether4
```

Agora, temos que criar outra bridge que conecta ao Tux22:

```shell
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


## EXP 3

Esta experiência é semelhante à anterior, a única diferença é que o Tux23 será ligado à bridge Y1.<br>
Para fazer isso, temos que fazer o seguinte:

Inicialmente, criar uma bridge que conecta o Tux23 e o Tux24:

```shell
interface bridge add name=bridgeY0
interface bridge port remove [find interface=ether3]
interface bridge port remove [find interface=ether4]
interface bridge port add bridge=bridgeY0 interface=ether3
interface bridge port add bridge=bridgeY0 interface=ether4
```

Depois, temos que criar uma ligação de cabos entre o Tux24 E1 e o Tux22 E0. Para isso ligamos o Tux24 E1 à switch, decidimos ligar ao eth5.
Agora temos que criar uma bridge que conecta o Tux22 e o Tux24.

```shell
interface bridge add name=bridgeY1
interface bridge port remove [find interface=ether2]
interface bridge port remove [find interface=ether5]
interface bridge port add bridge=bridgeY1 interface=ether2
interface bridge port add bridge=bridgeY1 interface=ether5
```

Agora configurar a outra interface do Tux24 para acomodar a ligação ao Tux22:

```shell
ifconfig eth1 172.16.21.253/24
```

Precisamos, também, de adicionar uma rota entre o Tux22 e o Tux23.

No Tux 22 fazemos o seguinte:

```shell
route add -net 172.16.20.0/24 gw 172.16.21.253
```

Onde `172.16.20.0/24` é a rede do destino (Tux23 neste caso) e `172.16.21.253` o gateway que servirá de conexão (Tux24 da rede do Tux22, neste caso)

No Tux 23 fazemos o seguinte:

```shell
route add -net 172.16.21.0/24 gw 172.16.20.254
```


## EXP 4

Primeiramente, temos de conectar o eth1 do **Router Console** ao **P2.1** e conectar o eth2 do **Route Console** a um port na **Switch**. Depois, trocamos o cabo que está ligado ao T4, que inicialmente estava conectado à **Switch**, para termos acesso à **Switch** e dar setup das rotas e bridges, e colocamos no **Route MT**, para termos acesso ao **RC**.