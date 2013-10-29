import processing.serial.*;                                                     // Einbinden der Serial Programmbibliothek
import cc.arduino.*;                                                            // Einbinden der Arduino Programmbibliothek
import ddf.minim.*;                                                             // Einbinden der Minim Programmbibliothek für die Soundausgabe

/*
  TwitterSaurus
  Dieses Programm sucht nach einem Suchbegriff auf Twitter. Wenn ein neuer Beitrag geschrieben wird, 
  wird ein Sound ausgelöst und ein Servo dreht sich. Er bewegt das Maul des Dinosauriers des Fritzing Creator Kits.
  
  Dieses Beispiel aus dem Fritzing Creator Kit: www.fritzing.org/creator-kit.
*/


String myQuery="Dino";                                                          // Begriff, nach dem gesucht werden soll

Twitter twitter;                                                                // Erzeugt ein Twitter-Objekt
Query query;                                                                    // Erzeugt ein Query-Objekt
Arduino arduino;                                                                // Erzeugt ein Arduino-Objekt
Minim minim;                                                                    // Erzeugt ein Minim-Objekt
AudioSample sound1;                                                             // Erzeugt ein AudioSample-Objekt
PFont font;                                                                     // Erzeugt ein PFont-Objekt

long timer;                                                                     // Timer wird angelegt
int timeout=1000;                                                               // Timeout – wie oft soll geprüft werden?

String lastMessage="";                                                          // speichert die letzte Twittermeldung
String currentMessage="";                                                       // speichert die aktuelle Twittermeldung

TwitterFactory twitterFactory;                                                  // Erstelle ein Twitter-Factory Objekt

int servoPin=10;                                                                // Servo an Arduino-Pin

void setup(){              
  size(400,200);                                                                // legt die Größe des Programmfensters fest
  ConfigurationBuilder cb = new ConfigurationBuilder();                         // ConfigurationBuilder für Twitter
  cb.setOAuthConsumerKey("uzr17kGYqhgNDjTBNQd1qA");                             // Hier muss Dein ConsumerKey eingegeben werden
  cb.setOAuthConsumerSecret("ZZUfCHruwv4d6Tn9uGz0UGebxfn4oQDikv3NeCbd14");      // Hier muss Dein ConsumerSecret eingegeben werden
  cb.setOAuthAccessToken("221060254-8MJIyVpXotDhemKaJKVN88L1FlCcndToB8y143LU"); // Hier muss Dein AccessToken eingegeben werden
  cb.setOAuthAccessTokenSecret("A2R7IkcSniMnBHYeRK02umHoIvrsHQAdx4NaMq6toY");   // Hier muss Dein AccessTokenSecret eingegeben werden

  twitterFactory = new TwitterFactory(cb.build());                              // öffnet eine Verbindung zu Twitter
  twitter = twitterFactory.getInstance();
  
  minim = new Minim(this);                                                      // Minim (Bibliothek für die Soundausgabe) wird initialisiert
  sound1 = minim.loadSample("sound1.wav");                                      // Sounddatei wird geladen
  font = loadFont("OCRAStd-14.vlw");                                            // Font-Datei wird geladen
  textFont(font);                                                               // Font wird initialisiert
  println(Arduino.list());                                                      // Alle seriellen Geräte werden in einer Liste ausgegeben, die Nummer für das Arduino muss 
  arduino = new Arduino(this, Arduino.list()[4]);                               // hier übergeben werden: Arduino.list()[nummer]
  arduino.pinMode(servoPin, Arduino.OUTPUT);                                    // Servo Pin wird im Arduino als Output festgelegt
}

void alertMe(){                                                                 // Methode wird aufgerufen, wenn es eine neue Nachricht gibt
  sound1.trigger();                                                             // Soundsample wird abgespielt
  arduino.analogWrite(servoPin, 120);                                           // Servo wird gedreht
  delay(500);                                                                   // warten
  arduino.analogWrite(servoPin, 20);                                            // Servo wird gedreht 
  delay(500);                                                                   // warten
}

void twitterConnect(){                                                          // Methode für die Verbindung zu Twitter
  try {                                                                         // versuche
    Query query = new Query(myQuery);
    QueryResult result = twitter.search(query);
    for (Status status : result.getTweets()) {
        println("@" + status.getUser().getScreenName() + ":" + status.getText());
        currentMessage="@" + status.getUser().getScreenName() + ":" + status.getText();
    }    
  }
  catch (TwitterException e) {                                                  // bei Fehler
    println("Couldn't connect: " + e);                                          // Fehlermeldung wird ausgegeben
  };
}

void draw(){                                                                    // draw Methode ist wie loop in Arduino
  if (millis()>timer+timeout){                                                  // wenn wartezeit vorüber
    twitterConnect();                                                           // Methodenaufruf
    timer=millis();                                                             // timer wird neu gestartet
  }

  if (currentMessage.equals(lastMessage) == false) {                            // wenn aktuelle Nachricht anders als letzte Nachricht ist 
    background(0);                                                              // Hindergrund füllen
    text(currentMessage, 10, 20,width-20,height-40);                            // Tweet als Text ausgeben
    alertMe();                                                                  // Methodenaufruf
    lastMessage=currentMessage;                                                 // letzteNachricht auf aktuelle Nachricht senden
  }
}

