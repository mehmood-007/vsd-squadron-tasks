
#include <uart-lib.h>
#include <gpio-lib.h>
#include <timer-lib.h>
#include <pwm-lib.h>
#include <string.h>
#include <stdlib.h>


void NMI_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void HardFault_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void Delay_Init(void);
void Delay_Ms(uint32_t n);
void led_off(void);
void led_on(void);


#define NUM_MODES 5

// Sleep in small chunks so a new UART byte can abort the animation.
// Returns 1 if aborted (byte pending), 0 if the full delay elapsed.
static uint8_t sleep_or_abort(uint32_t ms){
    while (ms > 0){
        if (usart1_available()) return 1;
        uint32_t step = (ms > 5) ? 5 : ms;
        Delay_Ms(step);
        ms -= step;
    }
    return 0;
}

static void mode_off(void){
    led_off();
    while (!usart1_available()){ Delay_Ms(10); }
}

static void cmd_led_help(void){
    usart1_write_string(
        "commands:\r\n"
        "  help              show this help\r\n"
        "  led on            turn LED on\r\n"
        "  led off           turn LED off\r\n"
        "  blink <ms>        blink at half-period <ms> (any key stops)\r\n"
        "  read <pin>        read digital pin, e.g. read C6\r\n"
        "  exit              leave shell\r\n");
}

static void cmd_blink(uint32_t half_ms){
    while (1){
        led_on();
        if (sleep_or_abort(half_ms)) { led_off(); return; }
        led_off();
        if (sleep_or_abort(half_ms)) return;
    }
}

static void cmd_read(const char *pin_str){
    int8_t v = gpio_read_pin(pin_str);
    if (v < 0){
        usart1_write_string("err: bad pin (use e.g. C6)\r\n");
        return;
    }
    char out[] = { pin_str[0], pin_str[1], '=', v ? '1' : '0', '\r', '\n', '\0' };
    usart1_write_string(out);
}

// Onboard LED on PC6 is active-high.
void led_on(void){  gpio_set(1); }
void led_off(void){ gpio_set(0); }

// Interactive command shell. Robust: unknown/invalid input -> error, keep prompt.
static void mode_blink(void){
    char line[48];
    cmd_led_help();
    while (1){
        usart1_write_string("> ");
        usart1_read_string(line, sizeof(line));

        char *tok[4] = {0};
        uint8_t n = 0;
        for (char *t = strtok(line, " \t"); t && n < 4; t = strtok(NULL, " \t")){
            tok[n++] = t;
        }
        if (n == 0) continue;

        if (strcmp(tok[0], "help") == 0){
            cmd_led_help();
        } else if (strcmp(tok[0], "exit") == 0){
            return;
        } else if (strcmp(tok[0], "led") == 0){
            if (n != 2){ usart1_write_string("err: usage: led on|off\r\n"); }
            else if (strcmp(tok[1], "on")  == 0){ led_on();  usart1_write_string("ok\r\n"); }
            else if (strcmp(tok[1], "off") == 0){ led_off(); usart1_write_string("ok\r\n"); }
            else { usart1_write_string("err: usage: led on|off\r\n"); }
        } else if (strcmp(tok[0], "blink") == 0){
            if (n != 2){ usart1_write_string("err: usage: blink <ms>\r\n"); continue; }
            int ms = atoi(tok[1]);
            if (ms <= 0 || ms > 10000){
                usart1_write_string("err: ms must be 1..10000\r\n");
            } else {
                cmd_blink((uint32_t)ms);
                led_off();
                usart1_write_string("stopped\r\n");
            }
        } else if (strcmp(tok[0], "read") == 0){
            if (n != 2){ usart1_write_string("err: usage: read <pin>\r\n"); }
            else { cmd_read(tok[1]); }
        } else {
            usart1_write_string("err: unknown cmd (try 'help')\r\n");
        }
    }
}

// Software PWM breathe. Commands: set duty <0..100>, set freq <Hz>, set speed <ms>.
static void mode_breathe(void){
    pwm_breathe_t pwm;
    char line[32];
    uint8_t line_len = 0;

    pwm_breathe_init(&pwm, 100, 100, 2400);
    timer_ms_init();
    usart1_write_string("breathe: set duty <0..100>, set freq <1..500>, set speed <100..60000>, exit\r\n");

    uint16_t next_tick = timer_ms() + 1;
    while (1){
        if (pwm_breathe_tick(&pwm)) led_on();
        else led_off();

        // Poll UART every iteration so bytes are captured before RDR overruns.
        while ((int16_t)(timer_ms() - next_tick) < 0){
            if (!usart1_available()) continue;
            char c = (char)usart1_read_2byte();
            if (c != '\r' && c != '\n'){
                if (line_len < sizeof(line) - 1) line[line_len++] = c;
                continue;
            }
            if (line_len == 0) continue;
            line[line_len] = '\0';
            line_len = 0;

            char *tokens[3] = {0};
            uint8_t count = 0;
            for (char *tok = strtok(line, " \t"); tok && count < 3; tok = strtok(NULL, " \t")){
                tokens[count++] = tok;
            }
            if (count == 0) continue;
            if (strcmp(tokens[0], "exit") == 0){
                led_off();
                return;
            }
            if (strcmp(tokens[0], "help") == 0){
                usart1_write_string("set duty <0..100> | set freq <1..500> | set speed <100..60000> | exit\r\n");
            } else if (count == 3 && strcmp(tokens[0], "set") == 0){
                int value = atoi(tokens[2]);
                uint8_t ok = 0;
                if (strcmp(tokens[1], "duty") == 0)       ok = pwm_breathe_set_duty(&pwm, value);
                else if (strcmp(tokens[1], "freq") == 0)  ok = pwm_breathe_set_freq(&pwm, value);
                else if (strcmp(tokens[1], "speed") == 0) ok = pwm_breathe_set_speed(&pwm, value);
                usart1_write_string(ok ? "ok\r\n" : "err: duty 0..100, freq 1..500, speed 100..60000\r\n");
            } else {
                usart1_write_string("err: use set duty|freq|speed <value> or exit\r\n");
            }
        }
        next_tick++;
    }
}

#define PATTERN_MAX 32

// Pattern sequencer: each '1'/'0' is one step of step_ms; loops while playing.
static void mode_pattern(void){
    static char pattern[PATTERN_MAX + 1] = "10110011";
    static uint16_t step_ms = 200;
    static const char *help =
        "pattern: set pattern <bits 0/1, max 32> <step ms 1..10000>, pattern play, pattern stop, exit\r\n";
    uint8_t playing = 0;
    uint8_t step = 0;
    uint16_t step_start = 0;
    char line[64];
    uint8_t line_len = 0;

    timer_ms_init();
    led_off();
    usart1_write_string(help);

    while (1){
        if (playing && (uint16_t)(timer_ms() - step_start) >= step_ms){
            step_start += step_ms;
            step++;
            if (pattern[step] == '\0') step = 0;
            if (pattern[step] == '1') led_on(); else led_off();
        }

        // Poll every iteration: USART1 has no RX FIFO.
        if (!usart1_available()) continue;
        char c = (char)usart1_read_2byte();
        if (c != '\r' && c != '\n'){
            if (line_len < sizeof(line) - 1) line[line_len++] = c;
            continue;
        }
        if (line_len == 0) continue;
        line[line_len] = '\0';
        line_len = 0;

        char *tokens[4] = {0};
        uint8_t count = 0;
        for (char *tok = strtok(line, " \t"); tok && count < 4; tok = strtok(NULL, " \t")){
            tokens[count++] = tok;
        }
        if (count == 0) continue;

        if (strcmp(tokens[0], "exit") == 0){
            led_off();
            return;
        } else if (strcmp(tokens[0], "help") == 0){
            usart1_write_string(help);
        } else if (count == 4 && strcmp(tokens[0], "set") == 0 && strcmp(tokens[1], "pattern") == 0){
            size_t len = strlen(tokens[2]);
            uint8_t valid = len > 0 && len <= PATTERN_MAX;
            for (size_t i = 0; valid && i < len; i++){
                valid = tokens[2][i] == '0' || tokens[2][i] == '1';
            }
            int ms = atoi(tokens[3]);
            if (!valid || ms < 1 || ms > 10000){
                usart1_write_string("err: bits must be 0/1 (1..32), ms 1..10000\r\n");
                continue;
            }
            memcpy(pattern, tokens[2], len + 1);
            step_ms = (uint16_t)ms;
            step = 0;
            step_start = timer_ms();
            if (playing){
                if (pattern[0] == '1') led_on(); else led_off();
            }
            usart1_write_string("ok\r\n");
        } else if (count == 2 && strcmp(tokens[0], "pattern") == 0 && strcmp(tokens[1], "play") == 0){
            playing = 1;
            step = 0;
            step_start = timer_ms();
            if (pattern[0] == '1') led_on(); else led_off();
            usart1_write_string("playing\r\n");
        } else if (count == 2 && strcmp(tokens[0], "pattern") == 0 && strcmp(tokens[1], "stop") == 0){
            playing = 0;
            led_off();
            usart1_write_string("stopped\r\n");
        } else {
            usart1_write_string("err: try 'help'\r\n");
        }
    }
}

// ----- Reaction-Time Game -----
// Reaction-time game: LED on after random delay, measure press latency, track best.
static void mode_game(void){
    static uint32_t best_ms = 0;      // 0 = no score yet
    static uint32_t rng     = 0xC0FFEEu;
    static uint8_t  hw_done = 0;

    // Re-init every entry: other modes (e.g. 'read C7') may reconfigure PC7.
    button_init();
    if (!hw_done){
        timer_ms_init();
        hw_done = 1;
    }

    usart1_write_string("== reaction time game ==\r\n");

    // DIAG: TIM2 should advance ~1000 during a 1000 ms SysTick delay.
    uint16_t cal = timer_ms();
    Delay_Ms(1000);
    usart1_write_string("timer check (expect ~1000): ");
    usart1_write_uint((uint16_t)(timer_ms() - cal));
    usart1_write_string("\r\n");

    usart1_write_string("press button (or any key) to start; type 'exit' to quit\r\n");

    while (1){
        led_off();

        // Wait for start: button press OR any UART input.
        button_wait_release();
        while (1){
            if (button_pressed_debounced()){
                button_wait_release();
                break;
            }
            if (usart1_available()){
                char line[16];
                usart1_read_string(line, sizeof(line));
                if (strcmp(line, "exit") == 0){
                    usart1_write_string("bye\r\n");
                    return;
                }
                break;
            }
            Delay_Ms(5);
        }

        usart1_write_string("wait for the LED...\r\n");

        // Randomized wait window 1000..5000 ms; stir with timer noise.
        rng = rng * 1664525u + 1013904223u + (uint32_t)timer_ms();
        uint32_t wait_ms = 1000u + (rng % 4000u);

        uint8_t false_start = 0;
        for (uint32_t t = 0; t < wait_ms; t += 5){
            if (button_pressed_debounced()){ false_start = 1; break; }
            Delay_Ms(5);
        }
        if (false_start){
            usart1_write_string("FALSE START! wait for the LED next time.\r\n\r\n");
            continue;
        }

        // Go: LED on, capture t0, wait for press.
        uint16_t t0 = timer_ms();
        led_on();
        uint8_t timed_out = 0;
        while (!button_pressed()){
            if ((uint16_t)(timer_ms() - t0) > 5000){ timed_out = 1; break; }
        }
        if (timed_out){
            led_off();
            usart1_write_string("timeout (>5000 ms), PC7 raw=");
            usart1_write_uint(GPIO_ReadInputDataBit(BTN_PORT, BTN_PIN));
            usart1_write_string("\r\n\r\n");
            continue;
        }
        uint32_t rt = (uint32_t)(timer_ms() - t0);
        led_off();

        usart1_write_string("reaction: ");
        usart1_write_uint(rt);
        usart1_write_string(" ms\r\n");

        if (best_ms == 0 || rt < best_ms){
            best_ms = rt;
            usart1_write_string("** new best! **\r\n");
        }
        usart1_write_string("best:     ");
        usart1_write_uint(best_ms);
        usart1_write_string(" ms\r\n\r\n");
    }
}

static void run_mode(uint8_t m){
    switch (m){
        case 0: 
            usart1_write_string("mode 0: off\r\n");     
            mode_off();     
            break;
        case 1: 
            usart1_write_string("mode 1: blink\r\n");   
            mode_blink();   
            break;
        case 2: 
            usart1_write_string("mode 2: breathe\r\n"); 
            mode_breathe(); 
            break;
        case 3: 
            usart1_write_string("mode 3: pattern\r\n"); 
            mode_pattern(); 
            break;
        case 4: 
            usart1_write_string("mode 4: reaction game\r\n");    
            mode_game();    
            break;
        default: usart1_write_string("unknown mode\r\n");   break;
    }
    led_off();
}

int main(void){
    char line[32];

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();
    gpio_init();
    led_off();
    usart1_init();


    usart1_write_string("ready. cmd: mode <0..4>\r\n");

    while (1){
        usart1_read_string(line, sizeof(line));

        // Split the line into space-separated tokens.
        char *tokens[4] = {0};
        uint8_t ntok = 0;
        for (char *t = strtok(line, " \t"); t && ntok < 4; t = strtok(NULL, " \t")){
            tokens[ntok++] = t;
        }

        char *arg = NULL;
        if (ntok >= 2 && strcmp(tokens[0], "mode") == 0){
            arg = tokens[1];
        } else if (ntok == 1 && tokens[0][0] >= '0' && tokens[0][0] <= '9'){
            arg = tokens[0];
        }

        if (arg){
            int m = atoi(arg);
            if (m >= 0 && m < NUM_MODES){
                run_mode((uint8_t)m);
                usart1_write_string("cmd: mode <0..4>\r\n");
                continue;
            }
        }
        usart1_write_string("usage: mode <0..4>\r\n");
    }
}

void NMI_Handler(void) {}
void HardFault_Handler(void)
{
    while (1){ }
}
