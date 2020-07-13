# WhatsAp2p

O WhatsApp p2p (peer to peer) é um projeto escrito em C que contém dois módulo (cliente e servidor) e que possibilita que os usuários possam trocam mensagens diretamente de máquinas distintas em uma mesma rede, sem a necessidade constante de um servidor intervindo na comunicação.

A aplicação permite criação de agenda de contato, grupos de conversas, envio de mensagens de texto e fotos.

# Cliente

Ao se conectar com o servidor, é autenticado como um usuário válido, e caso deseje conversar com alguem, requisita para o servidor o endereço de IP do mesmo, e inicia uma conversa direta.

# Servidor

Valida os usuários online guardando seus respectivos endereços de IP e realiza o envio de tal informação quando requisita pelo usuário que deseja iniciar um nova conversa.
