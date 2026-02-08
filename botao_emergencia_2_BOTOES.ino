#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiManager.h>
#include <Preferences.h>
#include <WebServer.h>

// Objeto para salvar dados permanentemente
Preferences preferences;

// Servidor Web na porta 80
WebServer server(80);

// Variáveis globais para configurações
String botToken = "";
String chatId = "";
String wifiSSID = "";
String wifiPassword = "";
bool enviarAudio = true;

// Pinos dos botões
const int BOTAO_EMERGENCIA = 2;  // GPIO2 = Emergência
const int BOTAO_CONFIG = 3;       // GPIO3 = Configuração

void setup() {
  Serial.begin(9600);
  delay(500);
  
  Serial.println("\n\n=================================");
  Serial.println("BOTAO DE EMERGENCIA ESP32-C3");
  Serial.println("=================================\n");

  // Configura pinos dos botões com pull-up interno
  pinMode(BOTAO_EMERGENCIA, INPUT_PULLUP);
  pinMode(BOTAO_CONFIG, INPUT_PULLUP);
  delay(50);

  // ===== VERIFICA BOTÃO DE CONFIGURAÇÃO =====
  if (digitalRead(BOTAO_CONFIG) == LOW) {
    Serial.println(">>> BOTÃO CONFIG PRESSIONADO!");
    Serial.println(">>> ENTRANDO EM MODO CONFIGURAÇÃO...\n");
    
    // Aguarda soltar o botão
    while (digitalRead(BOTAO_CONFIG) == LOW) {
      delay(10);
    }
    
    carregarConfiguracao();
    iniciarServidorWeb();
    
    // Loop infinito no modo config
    while(true) {
      server.handleClient();
      
      // Verifica comando RESET no Serial
      if (Serial.available() > 0) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        
        if (cmd == "RESET" || cmd == "reset") {
          Serial.println("\n>>> Apagando configurações...\n");
          apagarConfiguracao();
          delay(1000);
          ESP.restart();
        }
      }
      
      delay(10);
    }
  }

  // ===== VERIFICA BOTÃO DE EMERGÊNCIA =====
  if (digitalRead(BOTAO_EMERGENCIA) == LOW) {
    Serial.println("🚨 BOTÃO DE EMERGÊNCIA PRESSIONADO!");
    Serial.println(">>> EMERGÊNCIA ATIVADA!\n");
    
    // Aguarda soltar o botão
    while (digitalRead(BOTAO_EMERGENCIA) == LOW) {
      delay(10);
    }
    
    // Continua para enviar alerta
  } else {
    // Acordou mas botão não está mais pressionado
    Serial.println("Wake-up mas botão já solto");
    Serial.println("Voltando para deep sleep...\n");
    
    // Reconfigura wake-up e volta a dormir
    configurarWakeUp();
    esp_deep_sleep_start();
  }

  // Carrega configurações
  carregarConfiguracao();

  if (botToken == "" || chatId == "" || wifiSSID == "") {
    Serial.println(">>> Sem configuração! Abrindo portal...\n");
    abrirPortalConfiguracao();
  }

  // Conecta WiFi
  Serial.println("Conectando ao WiFi...");
  Serial.print("SSID: ");
  Serial.println(wifiSSID);
  
  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
  
  unsigned long startTime = millis();
  int dots = 0;
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
    delay(500);
    Serial.print(".");
    dots++;
    if (dots % 20 == 0) Serial.println();
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ WiFi conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    
    enviarAlertas();
  } else {
    Serial.println("\n✗ Falha na conexão WiFi!");
    Serial.println("Abrindo portal de configuração...\n");
    abrirPortalConfiguracao();
  }

  // Entra em Deep Sleep
  Serial.println("\n=================================");
  Serial.println("Entrando em Deep Sleep...");
  Serial.println("Consumo: ~10 µA");
  Serial.println();
  Serial.println("Botões:");
  Serial.println("  🚨 GPIO2 = EMERGÊNCIA");
  Serial.println("  ⚙️  GPIO3 = CONFIGURAÇÃO");
  Serial.println();
  Serial.println("Conexões:");
  Serial.println("  GPIO2 → Botão 1 → GND");
  Serial.println("  GPIO3 → Botão 2 → GND");
  Serial.println("=================================\n");
  delay(100);
  
  configurarWakeUp();
  esp_deep_sleep_start();
}

void loop() {
  // Verifica comando CONFIG no Serial
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    
    if (cmd == "CONFIG" || cmd == "config") {
      Serial.println("\n>>> CONFIG via Serial!\n");
      
      carregarConfiguracao();
      iniciarServidorWeb();
      
      while(true) {
        server.handleClient();
        
        if (Serial.available() > 0) {
          String resetCmd = Serial.readStringUntil('\n');
          resetCmd.trim();
          
          if (resetCmd == "RESET" || resetCmd == "reset") {
            Serial.println("\n>>> Apagando configurações...\n");
            apagarConfiguracao();
            delay(1000);
            ESP.restart();
          }
        }
        
        delay(10);
      }
    }
    
    if (cmd == "RESET" || cmd == "reset") {
      Serial.println("\n>>> Apagando configurações...\n");
      apagarConfiguracao();
      delay(1000);
      ESP.restart();
    }
  }
}

// ========== CONFIGURAÇÃO WAKE-UP ==========

void configurarWakeUp() {
  // Configura wake-up para AMBOS os botões
  // GPIO2 (emergência) OU GPIO3 (config)
  uint64_t wakeup_mask = (1ULL << BOTAO_EMERGENCIA) | (1ULL << BOTAO_CONFIG);
  esp_deep_sleep_enable_gpio_wakeup(wakeup_mask, ESP_GPIO_WAKEUP_GPIO_LOW);
  
  Serial.println("Wake-up configurado:");
  Serial.println("  GPIO2 ou GPIO3 = Acorda");
}

// ========== SERVIDOR WEB ==========

void iniciarServidorWeb() {
  if (wifiSSID != "" && wifiPassword != "") {
    Serial.println("Conectando WiFi salvo...");
    WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
    
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
      delay(500);
      Serial.print(".");
    }
    Serial.println();
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Criando Access Point...");
    WiFi.softAP("BotaoEmergencia_Config", "12345678");
    Serial.println("\n=================================");
    Serial.println("MODO CONFIGURAÇÃO ATIVO!");
    Serial.println("=================================");
    Serial.println("WiFi: BotaoEmergencia_Config");
    Serial.println("Senha: 12345678");
    Serial.print("Acesse: http://");
    Serial.println(WiFi.softAPIP());
    Serial.println("=================================\n");
  } else {
    Serial.println("\n=================================");
    Serial.println("SERVIDOR WEB ATIVO!");
    Serial.println("=================================");
    Serial.print("Acesse: http://");
    Serial.println(WiFi.localIP());
    Serial.println("=================================\n");
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/test", HTTP_GET, handleTest);
  
  server.begin();
  Serial.println("Servidor web iniciado!");
  Serial.println("Portal ativo até resetar.\n");
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Botão Emergência</title>";
  html += "<style>";
  html += "body{font-family:Arial,sans-serif;max-width:600px;margin:50px auto;padding:20px;background:#f5f5f5}";
  html += ".container{background:white;padding:30px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}";
  html += "h1{color:#d32f2f;text-align:center;margin-bottom:30px}";
  html += "label{display:block;margin:15px 0 5px;color:#333;font-weight:bold}";
  html += "input[type='text'],input[type='password']{width:100%;padding:10px;border:1px solid #ddd;border-radius:5px;box-sizing:border-box;font-size:14px}";
  html += "input[type='checkbox']{margin-right:10px}";
  html += ".checkbox-label{display:flex;align-items:center;margin:15px 0;font-weight:normal}";
  html += "button{width:100%;padding:12px;margin-top:20px;background:#4CAF50;color:white;border:none;border-radius:5px;cursor:pointer;font-size:16px}";
  html += "button:hover{background:#45a049}";
  html += ".btn-test{background:#2196F3;margin-top:10px}";
  html += ".btn-test:hover{background:#0b7dda}";
  html += ".info{background:#e3f2fd;padding:15px;border-radius:5px;margin-bottom:20px;font-size:13px;line-height:1.6}";
  html += ".current{background:#fff3e0;padding:10px;border-radius:5px;margin-bottom:20px;font-size:12px}";
  html += ".pins{background:#e8f5e9;padding:10px;border-radius:5px;margin-bottom:20px;font-size:12px}";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<h1>🚨 Configuração</h1>";
  
  html += "<div class='pins'>";
  html += "<strong>📌 Botões:</strong><br>";
  html += "GPIO2 = Emergência 🚨<br>";
  html += "GPIO3 = Configuração ⚙️";
  html += "</div>";
  
  html += "<div class='current'>";
  html += "<strong>📊 Config Atual:</strong><br>";
  html += "WiFi: " + (wifiSSID != "" ? wifiSSID : "Não configurado") + "<br>";
  html += "Token: " + (botToken != "" ? String(botToken.length()) + " chars" : "Vazio") + "<br>";
  html += "Chat ID: " + (chatId != "" ? chatId : "Vazio") + "<br>";
  html += "Áudio: " + String(enviarAudio ? "Ativado 🔊" : "Desativado 🔇");
  html += "</div>";
  
  html += "<div class='info'>";
  html += "<strong>📝 Como Configurar:</strong><br>";
  html += "1. Bot Token: @BotFather no Telegram<br>";
  html += "2. Chat ID: @userinfobot (pessoal) ou getUpdates (grupo)<br>";
  html += "3. Múltiplos: separe por vírgula (ex: 123, -456)<br>";
  html += "4. Clique em Salvar";
  html += "</div>";
  
  html += "<form action='/save' method='POST'>";
  html += "<label>📡 WiFi SSID:</label>";
  html += "<input type='text' name='ssid' value='" + wifiSSID + "' required>";
  html += "<label>🔒 WiFi Senha:</label>";
  html += "<input type='password' name='password' value='" + wifiPassword + "'>";
  html += "<label>🤖 Bot Token:</label>";
  html += "<input type='text' name='token' value='" + botToken + "' required placeholder='123456789:ABC...'>";
  html += "<label>💬 Chat ID (separe por vírgula):</label>";
  html += "<input type='text' name='chatid' value='" + chatId + "' required placeholder='123456, -789012'>";
  
  html += "<div class='checkbox-label'>";
  html += "<input type='checkbox' name='audio' value='1' " + String(enviarAudio ? "checked" : "") + ">";
  html += "<span>🔊 Enviar áudio de alarme</span>";
  html += "</div>";
  
  html += "<button type='submit'>💾 Salvar Configuração</button>";
  html += "</form>";
  html += "<button class='btn-test' onclick=\"location.href='/test'\">🧪 Testar Alerta</button>";
  html += "</div></body></html>";
  
  server.send(200, "text/html", html);
}

void handleSave() {
  wifiSSID = server.arg("ssid");
  wifiPassword = server.arg("password");
  botToken = server.arg("token");
  chatId = server.arg("chatid");
  enviarAudio = (server.arg("audio") == "1");
  
  salvarConfiguracao();
  
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta http-equiv='refresh' content='3;url=/'>";
  html += "<style>body{font-family:Arial;text-align:center;padding:50px;background:#f5f5f5}";
  html += ".success{background:white;padding:30px;border-radius:10px;max-width:400px;margin:0 auto}";
  html += "h1{color:#4CAF50}</style></head><body>";
  html += "<div class='success'><h1>✅ Salvo!</h1>";
  html += "<p>Configuração salva!</p>";
  html += "<p>Reiniciando em 3s...</p></div></body></html>";
  
  server.send(200, "text/html", html);
  
  delay(3000);
  ESP.restart();
}

void handleStatus() {
  String json = "{";
  json += "\"wifi\":\"" + wifiSSID + "\",";
  json += "\"token_len\":" + String(botToken.length()) + ",";
  json += "\"chat_id\":\"" + chatId + "\",";
  json += "\"audio\":" + String(enviarAudio ? "true" : "false");
  json += "}";
  
  server.send(200, "application/json", json);
}

void handleTest() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<style>body{font-family:Arial;text-align:center;padding:50px;background:#f5f5f5}";
  html += ".test{background:white;padding:30px;border-radius:10px;max-width:400px;margin:0 auto}";
  html += "</style></head><body><div class='test'>";
  html += "<h2>🧪 Testando...</h2>";
  
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      attempts++;
    }
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    enviarParaTodosDestinatarios("🧪 TESTE - Tudo OK!");
    html += "<p>✅ Mensagem enviada!</p>";
    html += "<p>Verifique o Telegram.</p>";
  } else {
    html += "<p>❌ Erro: WiFi não conectado</p>";
  }
  
  html += "<button onclick='history.back()'>Voltar</button>";
  html += "</div></body></html>";
  
  server.send(200, "text/html", html);
}

// ========== CONFIGURAÇÃO ==========

void abrirPortalConfiguracao() {
  WiFiManager wm;

  WiFiManagerParameter token_param("token", "Bot Token", botToken.c_str(), 50);
  WiFiManagerParameter chat_param("chatid", "Chat ID", chatId.c_str(), 50);
  WiFiManagerParameter info("<p>1. Token: @BotFather<br>2. Chat ID: @userinfobot</p>");

  wm.addParameter(&info);
  wm.addParameter(&token_param);
  wm.addParameter(&chat_param);
  wm.setConfigPortalTimeout(180);
  wm.setAPCallback(configModeCallback);

  Serial.println("Portal captivo...");
  
  if (!wm.autoConnect("BotaoEmergencia_Setup", "12345678")) {
    Serial.println("Timeout!");
    delay(3000);
    ESP.restart();
  }

  Serial.println("\n✓ WiFi conectado!");

  wifiSSID = WiFi.SSID();
  wifiPassword = WiFi.psk();
  botToken = token_param.getValue();
  chatId = chat_param.getValue();

  salvarConfiguracao();
  Serial.println("✓ Configuração salva!");
  delay(2000);
}

void configModeCallback(WiFiManager *wm) {
  Serial.println("\n=================================");
  Serial.println("PORTAL CAPTIVO ATIVO!");
  Serial.println("WiFi: BotaoEmergencia_Setup");
  Serial.println("Senha: 12345678");
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("=================================\n");
}

void carregarConfiguracao() {
  preferences.begin("emergencia", false);
  
  wifiSSID = preferences.getString("wifiSSID", "");
  wifiPassword = preferences.getString("wifiPass", "");
  botToken = preferences.getString("botToken", "");
  chatId = preferences.getString("chatId", "");
  enviarAudio = preferences.getBool("enviarAudio", true);
  
  preferences.end();

  Serial.println("📂 Configuração carregada:");
  Serial.println("  WiFi: " + (wifiSSID != "" ? wifiSSID : "VAZIO"));
  Serial.println("  Token: " + (botToken != "" ? String(botToken.length()) + " chars" : "VAZIO"));
  Serial.println("  Chat: " + (chatId != "" ? chatId : "VAZIO"));
  Serial.println("  Áudio: " + String(enviarAudio ? "ON" : "OFF"));
  Serial.println();
}

void salvarConfiguracao() {
  preferences.begin("emergencia", false);
  
  preferences.putString("wifiSSID", wifiSSID);
  preferences.putString("wifiPass", wifiPassword);
  preferences.putString("botToken", botToken);
  preferences.putString("chatId", chatId);
  preferences.putBool("enviarAudio", enviarAudio);
  
  preferences.end();
  Serial.println("✓ Configuração salva!");
}

void apagarConfiguracao() {
  preferences.begin("emergencia", false);
  preferences.clear();
  preferences.end();
  Serial.println("✓ Configurações apagadas!");
}

// ========== ALERTAS ==========

void enviarAlertas() {
  Serial.println("\n🚨🚨🚨 EMERGÊNCIA ACIONADA! 🚨🚨🚨\n");

  if (enviarAudio) {
    Serial.println("🔊 Enviando áudio de alarme...");
    enviarAudioAlarme();
    delay(1500);
  }

  Serial.println("📢 Enviando sequência de alertas...\n");
  
  enviarParaTodosDestinatarios("🚨 ALERTA 1/5 - EMERGÊNCIA!");
  delay(400);
  enviarParaTodosDestinatarios("🚨 ALERTA 2/5 - EMERGÊNCIA!");
  delay(400);
  enviarParaTodosDestinatarios("🚨 ALERTA 3/5 - EMERGÊNCIA!");
  delay(400);
  enviarParaTodosDestinatarios("🚨 ALERTA 4/5 - EMERGÊNCIA!");
  delay(400);
  enviarParaTodosDestinatarios("🚨🚨🚨 EMERGÊNCIA ACIONADA! 🚨🚨🚨");
  delay(1000);
  
  String timestamp = obterTimestamp();
  enviarParaTodosDestinatarios("⏰ " + timestamp + "\n📍 Botão de pânico pressionado!\n⚠️ VERIFIQUE IMEDIATAMENTE!");

  Serial.println("\n✅ Todos os alertas enviados!");
  Serial.println("   - Áudio: " + String(enviarAudio ? "Sim 🔊" : "Não 🔇"));
  Serial.println("   - Mensagens: 6");
  Serial.println();
}

void enviarAudioAlarme() {
  String ids = chatId;
  ids.trim();
  
  int start = 0;
  int comma = 0;
  
  while (comma != -1) {
    comma = ids.indexOf(',', start);
    
    String id;
    if (comma == -1) {
      id = ids.substring(start);
    } else {
      id = ids.substring(start, comma);
    }
    
    id.trim();
    
    if (id.length() > 0) {
      enviarAudioParaChat(id);
      delay(300);
    }
    
    start = comma + 1;
  }
}

void enviarAudioParaChat(String chatIdDestino) {
  if (WiFi.status() != WL_CONNECTED) return;
  
  HTTPClient http;
  
  String audioUrl = "https://www.soundjay.com/misc/sounds/bell-ringing-05.mp3";
  audioUrl.replace(":", "%3A");
  audioUrl.replace("/", "%2F");
  
  String url = "https://api.telegram.org/bot" + botToken + 
               "/sendAudio?chat_id=" + chatIdDestino + 
               "&audio=" + audioUrl +
               "&caption=%F0%9F%9A%A8%20SOM%20DE%20ALERTA!";
  
  http.begin(url);
  http.setTimeout(15000);
  
  int code = http.GET();
  
  if (code == 200) {
    Serial.println("  ✓ Áudio → " + chatIdDestino);
  } else {
    Serial.println("  ⚠️ Erro áudio (" + String(code) + ")");
  }
  
  http.end();
}

void enviarParaTodosDestinatarios(String msg) {
  String ids = chatId;
  ids.trim();
  
  int start = 0;
  int comma = 0;
  
  while (comma != -1) {
    comma = ids.indexOf(',', start);
    
    String id;
    if (comma == -1) {
      id = ids.substring(start);
    } else {
      id = ids.substring(start, comma);
    }
    
    id.trim();
    
    if (id.length() > 0) {
      enviarMensagemTelegram(msg, id);
      delay(200);
    }
    
    start = comma + 1;
  }
}

void enviarMensagemTelegram(String msg, String chatIdDestino) {
  if (WiFi.status() != WL_CONNECTED) return;
  
  HTTPClient http;
  
  msg.replace(" ", "%20");
  msg.replace("!", "%21");
  msg.replace(":", "%3A");
  msg.replace("\n", "%0A");
  msg.replace("🚨", "%F0%9F%9A%A8");
  msg.replace("⏰", "%E2%8F%B0");
  msg.replace("📍", "%F0%9F%93%8D");
  msg.replace("⚠️", "%E2%9A%A0%EF%B8%8F");
  msg.replace("🧪", "%F0%9F%A7%AA");
  
  String url = "https://api.telegram.org/bot" + botToken + 
               "/sendMessage?chat_id=" + chatIdDestino + 
               "&text=" + msg +
               "&disable_notification=false";
  
  http.begin(url);
  http.setTimeout(10000);
  
  int code = http.GET();
  
  if (code == 200) {
    Serial.println("  ✓ Msg → " + chatIdDestino);
  } else {
    Serial.println("  ✗ Erro " + String(code));
  }
  
  http.end();
}

String obterTimestamp() {
  unsigned long t = millis() / 1000;
  int h = t / 3600;
  int m = (t % 3600) / 60;
  int s = t % 60;
  
  char buf[30];
  sprintf(buf, "%02dh%02dm%02ds desde boot", h, m, s);
  return String(buf);
}
