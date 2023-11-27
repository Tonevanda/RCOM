# Guia para desenvolvimento da aplicação de download

A função de download será chamada com o seguinte formato:

```shell
./download ftp://[<user>:<password>@]<host>/<url-path>
```

Primeiramente, temos de nos conectar ao servidor. Para isso, temos de descontruir a string passada como input e utilizamos o parâmetro `host` (o user e password são opcionais, caso não sejam passados o user é anonymous e password é anonymous).

Com o nome do servidor, temos de abrir um socket com o seu IP. Para obtermos o IP chamamos a função do ficheiro `getip.c`, que vai devolver o IP do host pelo nome.

Agora que temos o IP do `host`, temos que criar uma socket com esse IP para nos ligarmos ao servidor. Para isso utilizamos a função do ficheiro `clientTCP.c`.

Agora, estamos conectados ao servidor e temos os valores necessários para lhe fazer um pedido:

    - `user` (anonymous, se não for passado como input)
    - `password` (anonymous, se não for passada como input)
    - `url-path`, que representa o ficheiro para ser transferido do servidor

Agora, temos que enviar esses parâmetros para o servidor através de uma state machine, constantemente checkando se a resposta é 203, caso contrário terminamos o programa.

