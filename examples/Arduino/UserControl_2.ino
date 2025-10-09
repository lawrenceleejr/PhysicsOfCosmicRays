
#define Button_Pin  6
#define Slider_Pin  A0

int SliderValue;
int ButtonValue;

void setup() {


Serial.begin(9600);

}

void loop() {

SliderValue = analogRead(Slider_Pin);

Serial.print("Slider Value: "); 
Serial.println(SliderValue);

ButtonValue = digitalRead(Button_Pin);

Serial.print("Button Value: ");
Serial.println(ButtonValue);

delay(100);
}
