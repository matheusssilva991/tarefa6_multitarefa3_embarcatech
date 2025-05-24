# **Sistema de gerenciamento de ficha de RU**

## **Descrição**

Este projeto é um sistema embarcado para gerenciamento de fichas de restaurante universitário (RU), utilizando Raspberry Pi Pico, FreeRTOS, display OLED, botões, LED RGB e buzzer. O sistema controla a quantidade de fichas disponíveis, permitindo retirada, devolução e reset, com feedback visual e sonoro.

---

## **Funcionalidades**

- **Gerenciamento de Fichas:**
  - Controle do número de fichas disponíveis via semáforo de contagem.
  - Retirada (entrada) e devolução (saída) de fichas por botões físicos.
  - Reset rápido para restaurar todas as fichas.
- **Interface Visual e Sonora:**
  - Display OLED mostra total, livres e usadas.
  - LED RGB indica o status das fichas (azul: todas livres, vermelho: nenhuma livre, amarelo: uma livre, verde: parcial).
  - Buzzer emite sons em caso de erro (sem fichas) ou reset.
- **Debounce por Software:**
  - Implementado para evitar múltiplos acionamentos por ruído nos botões.

---

## **Requisitos**

### **Hardware**

- Raspberry Pi Pico
- Display OLED SSD1306 (128x64 pixels, I2C)
- 3 botões (entrada, saída, reset)
- LED RGB
- Buzzer
- Resistores e jumpers

### **Software**

- **SDK do Raspberry Pi Pico**
- **FreeRTOS** para multitarefa
- **Bibliotecas adicionais:**
  - `ssd1306` para display OLED
  - `button` para leitura dos botões
  - `buzzer` para controle do buzzer
  - `led` para controle do LED RGB

---

## **Como Rodar**

1. **Clone o repositório:**

   ```bash
   git clone https://github.com/matheusssilva991/tarefa6_multitarefa3_embarcatech.git
   cd ru-token-manager
   ```

2. **Configure o caminho do FreeRTOS:**
   - Edite o arquivo `CMakeLists.txt` e defina o caminho correto para o FreeRTOS:

   ```cmake
   set(FREERTOS_PATH "/caminho/para/seu/FreeRTOS")
   ```

   - Certifique-se de que o FreeRTOS esteja corretamente instalado e acessível.

3. **Compile e envie o código para o Raspberry Pi Pico:**

   ```bash
   mkdir build
   cd build
   cmake -G "Ninja" ..
   ninja
   ```

4. **Conecte o hardware e alimente o Raspberry Pi Pico.**
   - O sistema iniciará automaticamente e exibirá os dados no display OLED.

---

## **Demonstração**

▶️ [Assista ao vídeo de demonstração no Google Drive](https://drive.google.com/file/d/1UNcEwTQXoDbNuP66fNSVNQDA4XNAGawN/view?usp=drive_link)

---

## **Contribuindo**

1. Faça um fork deste repositório.
2. Crie uma nova branch: `git checkout -b minha-contribuicao`.
3. Faça suas alterações e commit: `git commit -m 'Minha contribuição'`.
4. Envie para o seu fork: `git push origin minha-contribuicao`.
5. Abra um Pull Request neste repositório.

---

## **Licença**

Este projeto está licenciado sob a licença MIT. Veja o arquivo [LICENSE](LICENSE) para mais detalhes.
