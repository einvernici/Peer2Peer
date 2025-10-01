📂 P2P File Sharing (UDP em C)

Este projeto implementa um sistema simples de compartilhamento de arquivos P2P usando sockets UDP e threads.
Cada peer atua ao mesmo tempo como cliente e servidor, podendo enviar e receber arquivos em uma rede local.

⚙️ Funcionalidades

📥 Adicionar arquivo (add <nomedoarquivo>)

Notifica outros peers que um arquivo foi criado.

O peer dono envia o conteúdo quando requisitado.

🗑️ Remover arquivo (del <nomedoarquivo>)

Remove o arquivo localmente e notifica os peers para deletarem também.

📃 Listar arquivos (list)

Pede a lista de arquivos de outros peers.

Responde com a lista de arquivos disponíveis em sua pasta ./p2p.

📡 Sincronização automática

Quando um peer adiciona um arquivo, os outros podem requisitar o conteúdo automaticamente via protocolo (ADD → GET → FILE).
