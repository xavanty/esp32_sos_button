# ESP32 Botão de Emergência

Sistema de botão de pânico emergencial para idosos usando ESP32-C3 Super Mini com notificações via Telegram.

## Visão Geral

Este projeto implementa um sistema de botão de emergência com consumo ultra-baixo de energia que envia alertas instantâneos via Telegram quando acionado. O dispositivo permanece em modo deep sleep consumindo apenas ~10µA, tornando-o ideal para aplicações alimentadas por bateria.

## Funcionalidades

- **Consumo Ultra-Baixo de Energia**: ~10µA em modo deep sleep
- **Sistema de Dois Botões**:
  - GPIO2: Botão de Emergência/Pânico
  - GPIO3: Botão de Configuração
- **Integração com Telegram**:
  - Suporte a múltiplos destinatários (Chat IDs separados por vírgula)
  - Notificações com alarme sonoro
  - Múltiplas mensagens de alerta sequenciais
  - Funcionalidade de teste
- **Configuração via Web**:
  - Configuração de credenciais WiFi
  - Configuração do Token do Bot e Chat ID do Telegram
  - Ativação/desativação do alarme sonoro
  - Portal de configuração ao vivo
- **Portal Cativo**: Configuração automática na primeira inicialização ou quando a configuração está ausente
- **Armazenamento Persistente**: Configuração salva nas Preferences (NVS) do ESP32

## Requisitos de Hardware

- ESP32-C3 Super Mini
- 2x Botões de pressão (normalmente abertos)
- Bateria ou fonte de alimentação
- (Opcional) PCB personalizada - arquivos de projeto Eagle incluídos

## Configuração dos Pinos

| GPIO | Função | Conexão |
|------|--------|---------|
| GPIO2 | Botão de Emergência | Botão → GND |
| GPIO3 | Botão de Configuração | Botão → GND |

Ambos os pinos utilizam resistores pull-up internos e despertam o dispositivo com sinal LOW.

## Requisitos de Software

### Bibliotecas Arduino

Instale estas bibliotecas pelo Gerenciador de Bibliotecas do Arduino:

- `WiFi` (integrada)
- `HTTPClient` (integrada)
- `WiFiManager` por tzapu
- `Preferences` (integrada)
- `WebServer` (integrada)

### Configuração do Bot do Telegram

1. Abra o Telegram e procure por [@BotFather](https://t.me/BotFather)
2. Envie o comando `/newbot` e siga as instruções para criar seu bot
3. Ao final, o BotFather enviará o **API Token** do seu bot (formato: `123456789:ABCdefGHIjklMNO...`). **Copie e guarde este token**, pois ele será necessário na configuração do dispositivo
4. Obtenha seu Chat ID:
   - **Recomendado: Crie um grupo no Telegram** para receber as notificações de emergência
   - Adicione o seu bot ao grupo
   - Adicione ao grupo todas as pessoas que desejam receber as notificações de emergência (familiares, cuidadores, etc.)
   - Envie qualquer mensagem no grupo
   - Em seguida, acesse a seguinte URL no navegador, usando o token coletado no passo 3:
     ```
     https://api.telegram.org/bot<SEU_TOKEN_DO_PASSO_3>/getUpdates
     ```
   - No resultado JSON, procure pelo campo `"chat":{"id":` — esse número é o **Chat ID do grupo**
   - **Atenção**: o ID de grupo sempre começa com `-` (sinal de menos), por exemplo: `-1001234567890`. Inclua o `-` ao configurar o dispositivo

## Instalação

1. Clone este repositório:
   ```bash
   git clone https://github.com/yourusername/esp32_sos_button.git
   ```

2. Abra o arquivo `botao_emergencia_2_BOTOES.ino` na Arduino IDE

3. Selecione a placa: **ESP32C3 Dev Module**

4. Faça o upload do código para o seu ESP32-C3

## Configuração

### Primeira Configuração

1. Ligue o dispositivo — como ainda não há configuração de WiFi salva, ele entra automaticamente no modo de configuração
2. Conecte-se à rede WiFi: `BotaoEmergencia_Config` (senha: `12345678`)
3. Acesse o portal web pelo IP mostrado no Monitor Serial
4. Insira suas configurações:
   - SSID e Senha do WiFi
   - Token do Bot do Telegram
   - Chat ID(s) - múltiplos IDs separados por vírgula
   - Preferência de alarme sonoro

### Via Comandos Serial

Você também pode configurar pelo Monitor Serial (9600 baud):

- `CONFIG` - Entrar no modo de configuração
- `RESET` - Limpar todas as configurações salvas

## Uso

### Operação Normal

1. O dispositivo entra em deep sleep após a configuração
2. Pressione o **Botão de Emergência (GPIO2)** para acionar o alerta
3. O dispositivo acorda, conecta ao WiFi e envia:
   - Alarme sonoro (se habilitado)
   - 5 mensagens de aviso sequenciais
   - Alerta final detalhado com data e hora

### Sequência de Alerta

Quando o botão de emergência é pressionado:
```
🔊 Alarme sonoro (opcional)
🚨 ALERTA 1/5 - EMERGÊNCIA!
🚨 ALERTA 2/5 - EMERGÊNCIA!
🚨 ALERTA 3/5 - EMERGÊNCIA!
🚨 ALERTA 4/5 - EMERGÊNCIA!
🚨🚨🚨 EMERGÊNCIA ACIONADA! 🚨🚨🚨
⏰ [data e hora]
📍 Botão de pânico pressionado!
⚠️ VERIFIQUE IMEDIATAMENTE!
```

### Funcionalidades do Portal de Configuração

A interface web oferece:
- Exibição da configuração atual
- Configuração de credenciais WiFi
- Configuração do bot do Telegram
- Suporte a múltiplos destinatários
- Ativação/desativação do alarme sonoro
- Botão de teste para verificar a configuração
- Endpoint de status da configuração (`/status`)

## Projeto da PCB

O repositório inclui arquivos Eagle CAD para uma PCB personalizada:

- `placa.sch` - Esquemático
- `placa.brd` - Layout da placa
- `ESP32C3supermini.lbr` - Biblioteca personalizada do ESP32-C3 Super Mini

## Consumo de Energia

- Deep Sleep: ~10µA
- Ativo (WiFi): ~80-120mA (apenas por breves períodos)
- Estimativa de duração da bateria: Vários meses com uma única CR2032 (dependendo da frequência de uso)

## Solução de Problemas

### O dispositivo não conecta ao WiFi
- Mantenha pressionado o Botão de Configuração (GPIO3) ao ligar
- Conecte-se ao AP `BotaoEmergencia_Config`
- Reconfigure as definições de WiFi

### Telegram não está recebendo mensagens
- Verifique se o Token do Bot está correto
- Certifique-se de que o Chat ID é válido
- Verifique se o bot tem permissão para enviar mensagens ao usuário/grupo
- Use o botão de Teste no portal de configuração

### Restaurar para configurações de fábrica
- Envie o comando `RESET` pelo Monitor Serial (9600 baud)
- Ou mantenha o Botão de Configuração pressionado e envie `RESET`

## Licença

Este projeto está licenciado sob a Licença Pública Geral GNU v3.0 - veja o arquivo [LICENSE](LICENSE) para detalhes.

## Contribuições

Contribuições são bem-vindas! Sinta-se à vontade para enviar um Pull Request.

## Aviso de Segurança

Este dispositivo é destinado como uma medida de segurança adicional e não deve ser o único sistema de emergência. Sempre garanta que métodos adequados de contato com serviços de emergência estejam disponíveis.

## Autor

Criado para aplicações de cuidado de idosos e resposta a emergências.

## Agradecimentos

- Construído na plataforma ESP32-C3
- Utiliza a biblioteca WiFiManager para configuração fácil
- API de Bot do Telegram para notificações
