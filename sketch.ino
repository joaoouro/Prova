#include <WiFi.h>
#include <HTTPClient.h>

// Definições dos pinos dos LEDs
#define led_verde 2
#define led_vermelho 40
#define led_amarelo 9

// Definição do pino do botão
const int pino_botao = 18;

// Variáveis para o gerenciamento do botão e debounce
int estado_do_botao = HIGH;
int ultima_leitura_botao = HIGH;
unsigned long ultimo_debounce = 0; 
unsigned long debounce_delay = 50; // Delay do debounce do botão

// Definições do pino e limiar do LDR
const int pino_ldr = 4;  
int limiar = 600; // Limiar para determinar se está escuro ou claro

// Variáveis de controle de tempo
int contador_pressionamentos = 0;
unsigned long tempo_anterior = 0; 
unsigned long tempo_piscar = 1000;  // Intervalo para piscar o LED amarelo no modo noturno
unsigned long tempo_verde = 3000;    // Tempo do LED verde no modo convencional
unsigned long tempo_amarelo = 2000;  // Tempo do LED amarelo no modo convencional
unsigned long tempo_vermelho = 5000; // Tempo do LED vermelho no modo convencional

void setup() {
  Serial.begin(115200);

  // Configuração dos pinos de saída (LEDs)
  pinMode(led_amarelo, OUTPUT);
  pinMode(led_verde, OUTPUT);
  pinMode(led_vermelho, OUTPUT);

  // Configuração da entrada do botão com resistor pull-up interno
  pinMode(pino_botao, INPUT_PULLUP);

  // Configuração da entrada do LDR
  pinMode(pino_ldr, INPUT);

  // Garantir que os LEDs iniciem apagados
  digitalWrite(led_amarelo, LOW);
  digitalWrite(led_verde, LOW);
  digitalWrite(led_vermelho, LOW);

  // Conexão à rede WiFi
  WiFi.begin("Wokwi-GUEST", "");
  delay(1000);

  unsigned long startAttemptTime = millis();
  unsigned long connectionTimeout = 20000; // 20 segundos de timeout
  Serial.println("Tentando conectar ao WiFi...");
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < connectionTimeout) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Conectado ao WiFi com sucesso!");
    Serial.print("Endereço IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Falha na conexão ao WiFi.");
  }
}

void loop() {
  // Leitura do botão com debounce
  int leitura = digitalRead(pino_botao);
  if (leitura != ultima_leitura_botao) {
    ultimo_debounce = millis();
  }

  if ((millis() - ultimo_debounce) > debounce_delay) {
    if (leitura != estado_do_botao) {
      estado_do_botao = leitura;
      // Botão pressionado (LOW)
      if (estado_do_botao == LOW) {
        contador_pressionamentos++;

        // Leitura do LDR
        int status_ldr = analogRead(pino_ldr);

        // Se estiver claro e o semáforo estiver no vermelho
        if (status_ldr > limiar && digitalRead(led_vermelho) == HIGH) {
          // Abre o semáforo após 1 segundo
          delay(1000);
          digitalWrite(led_vermelho, LOW);
          digitalWrite(led_verde, HIGH);
          contador_pressionamentos = 0;
        }

        // Caso o botão seja pressionado 3 vezes nessas condições (claro + vermelho aceso)
        // Enviar requisição HTTP
        if (contador_pressionamentos == 3) {
          if (WiFi.status() == WL_CONNECTED) {
            HTTPClient http;
            String serverPath = "http://www.google.com.br/";
            http.begin(serverPath.c_str());
            int httpResponseCode = http.GET();
            if (httpResponseCode > 0) {
              Serial.print("Código de resposta HTTP: ");
              Serial.println(httpResponseCode);
              String payload = http.getString();
              Serial.println(payload);
            } else {
              Serial.print("Erro na requisição HTTP. Código: ");
              Serial.println(httpResponseCode);
            }
            http.end();
          } else {
            Serial.println("WiFi desconectado, não foi possível enviar alerta.");
          }
          contador_pressionamentos = 0;
        }
      }
    }
  }

  ultima_leitura_botao = leitura;

  // Leitura do LDR para determinar modo noturno ou convencional
  int status_ldr = analogRead(pino_ldr);

  // Modo noturno: piscar LED amarelo a cada segundo (sem "firula")
  if (status_ldr <= limiar) {
    unsigned long tempo_atual = millis();
    if (tempo_atual - tempo_anterior >= tempo_piscar) {
      tempo_anterior = tempo_atual;
      digitalWrite(led_amarelo, !digitalRead(led_amarelo));
      digitalWrite(led_verde, LOW);
      digitalWrite(led_vermelho, LOW);
    }
  } else {
    // Modo convencional: verde (3s), amarelo (2s), vermelho (5s)
    unsigned long tempo_atual = millis();
    if (tempo_atual - tempo_anterior >= tempo_verde) {
      // Verde
      digitalWrite(led_verde, HIGH);
      digitalWrite(led_amarelo, LOW);
      digitalWrite(led_vermelho, LOW);
      delay(3000);

      // Amarelo
      digitalWrite(led_verde, LOW);
      digitalWrite(led_amarelo, HIGH);
      delay(2000);

      // Vermelho
      digitalWrite(led_amarelo, LOW);
      digitalWrite(led_vermelho, HIGH);
      delay(5000);

      tempo_anterior = millis();
    }
  }
}
