# Rede P2P UDP em C

Este projeto implementa um sistema simples de compartilhamento de arquivos P2P usando sockets UDP e threads.
Cada peer atua ao mesmo tempo como cliente e servidor, podendo enviar e receber arquivos em uma rede local.

# Funcionalidades:

## Adicionar arquivo (add 'nome do arquivo')

- Notifica outros peers que um arquivo foi criado.
- Quando um peer adiciona um arquivo, os outros podem requisitar o conteúdo automaticamente via protocolo (ADD → GET → FILE).

## Remover arquivo (del 'nome do arquivo')

- Remove o arquivo localmente e notifica os peers para deletarem também.

## Listar arquivos (list)

- Pede a lista de arquivos de outros peers.
- Responde com a lista de arquivos disponíveis em sua pasta ./p2p.

## Como Testar o Programa:

- O programa foi criado e testado usando Virtual Machines. Cada Peer deve rodar o código em sua própria VM.

1. Setup Virtual Machine:

- Uma VM deve ser criada para cada Peer da rede. A configuração de rede deve ser "placa em modo bridge" para que a VM tenha um IP próprio e possa enviar enviar e receber mensagens através da rede.
- O IP pode ser encontrado usando o comando ifconfig.

2. Alterações do código:

- O arquivo peer.c deve ser modificado para constar quais são os IPs dos Peers da rede - cada Peer deve adicionar o IP dos outros Peers. 

3. Compilar e Rodar em Todos os Peers

- Compilar: gcc peer.c -o peer
- Rodar: ./peer
- O projeto assume que todos os Peers estejam conectados ao mesmo tempo. Portanto, todos os Peers devem estar rodando o programa no momento do teste.
- Para encerrar o programa Ctrl + C.
