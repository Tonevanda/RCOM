# Guia para desenvolvimento da aplicação de download

A função de download será chamada com o seguinte formato:

```shell
./download ftp://[<user>:<password>@]<host>/<url-path>
```

Primeiramente, temos de nos conectar ao servidor. Para isso, temos de desconstruir a string passada como input e utilizar o parâmetro `host` (o user e password são opcionais, caso não sejam passados o user é anonymous e password é anonymous).

## Parsing da string 

Para dar parse da string, podemos:

    - Ver se há um `@`, se sim, quer dizer que foi inserido user e password. O user é tudo que está entre o `//` e os `:` e a password é tudo que está entre `:` e o `@`.
    - O `host` é o que está depois do `@`, se existir, caso contrário é o que está depois do `//`, e está antes da última `/`
    - O `url-path` é tudo o que está após a última `/`

## Conectar ao servidor

O protocolo telnet têm 2 sockets diferentes:

    - Socket de controlo, que dá handle de autenticação e responde aos pedidos
    - Socket de dados, que será revelado caso o socket de controlo dê a permissão

Com o nome do servidor, temos de abrir um socket com o seu IP. Para obtermos o IP chamamos a função `getIP()`, que vai devolver o IP do host pelo nome.

Agora que temos o IP do `host`, temos que criar uma socket com esse IP para nos ligarmos ao servidor. Para isso utilizamos a função `connectToServer()`.

Agora, estamos conectados ao servidor através de um socket de controlo e temos os valores necessários para lhe fazer um pedido:

    - `user` (anonymous, se não for passado como input)
    - `password` (anonymous, se não for passada como input)
    - `url-path`, que representa o ficheiro para ser transferido do servidor

Temos que enviar o `user` e `password` para o socket de controlo e, se a autenticação for bem sucedida, colocamos em modo passivo.<br>
Após isso, o socket de controlo irá responder com uma resposta de 6 bytes:

    - Os primeiros 4 bytes representam um IP
    - Os últimos 2 bytes representam uma porta desse IP

Essa resposta de controlo representa o IP e porta onde estão armazenados os ficheiros desse servidor.<br>

Temos, agora, de abrir um socket com esse IP e porta, que representa o socket de dados. Para isso, podemos utilizar a função `connectToServer()`, que tinhamos utilizado anteriormente para abrir o socket de controlo.

## Fazer download do ficheiro

Temos, agora, de enviar um pedido retrieve para o socket de controlo com o `url-path`, passado como parâmetro inicialmente.<br>
Se esse pedido for bem sucedido, o socket de dados terá esse ficheiro que foi pedido.

É só agora necessário ler desse socket de dados e escrever para um ficheiro local na nossa máquina.

## Notas para desenvolvimento

Ao contrário do projeto anterior, não temos que ler e escrever byte a byte, quando lemos e escrevemos informação fazemos tudo ao mesmo tempo.
