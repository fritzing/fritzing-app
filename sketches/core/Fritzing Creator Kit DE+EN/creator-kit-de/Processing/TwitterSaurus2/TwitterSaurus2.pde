/*
 
Just a simple Processing and Twitter thingy majiggy
 
RobotGrrl.com
 
Code licensed under:
CC-BY
 
*/
 
// First step is to register your Twitter application at dev.twitter.com
// Once registered, you will have the info for the OAuth tokens
// You can get the Access token info by clicking on the button on the
// right on your twitter app's page
// Good luck, and have fun!
 
// This is where you enter your Oauth info
static String OAuthConsumerKey = "uzr17kGYqhgNDjTBNQd1qA";
static String OAuthConsumerSecret = "ZZUfCHruwv4d6Tn9uGz0UGebxfn4oQDikv3NeCbd14";
 
// This is where you enter your Access Token info
static String AccessToken = "221060254-8MJIyVpXotDhemKaJKVN88L1FlCcndToB8y143LU";
static String AccessTokenSecret = "A2R7IkcSniMnBHYeRK02umHoIvrsHQAdx4NaMq6toY";
 
// Just some random variables kicking around
String myTimeline;
java.util.List statuses = null;
User[] friends;
TwitterFactory twitterFactory;
//Twitter twitter;

    Twitter twitter = TwitterFactory.getSingleton();


RequestToken requestToken;
String[] theSearchTweets = new String[11];
 
 
 
void setup() {
 
size(100,100);
background(0);
 
connectTwitter();
//sendTweet("Hey from Simple Processing woop woop #loadedsith #robotgirl");
//getTimeline();
getSearchTweets(); 
 
}
 
 
void draw() {
 
background(0);
 
}
 
 
// Initial connection
void connectTwitter() {
ConfigurationBuilder cb = new ConfigurationBuilder();
cb.setOAuthConsumerKey(OAuthConsumerKey);
cb.setOAuthConsumerSecret( OAuthConsumerSecret );
cb.setOAuthAccessToken( AccessToken);
cb.setOAuthAccessTokenSecret( AccessTokenSecret );
twitterFactory = new TwitterFactory(cb.build());
twitter = twitterFactory.getInstance();
println("connected");
}
 
// Sending a tweet
void sendTweet(String t) {
 
try {
Status status = twitter.updateStatus(t);
println("Successfully updated the status to [" + status.getText() + "].");
} catch(TwitterException e) {
println("Send tweet: " + e + " Status code: " + e.getStatusCode());
}
 
}
 
 
// Loading up the access token
private static AccessToken loadAccessToken(){
return new AccessToken(AccessToken, AccessTokenSecret);
}
 
 
// Get your tweets
void getTimeline() {
 
try {
statuses = twitter.getUserTimeline();
} catch(TwitterException e) {
println("Get timeline: " + e + " Status code: " + e.getStatusCode());
}
 
for(int i=0; i<statuses.size(); i++) {
Status status = (Status)statuses.get(i);
println(status.getUser().getName() + ": " + status.getText());
}
 
}
 
 
// Search for tweets
void getSearchTweets() {
String queryStr = "Fritzing";
 
try {
//Query query = new Query("source:twitter4j yusukey");
//query.setRpp(10); // Get 10 of the 100 search results
/*QueryResult result = twitter.search(query);
for (Status status : result.getStatus()) {
println("@" + status.getUser().getScreenName() + ":" + status.getText());
*/
    Query query = new Query("hi");
    QueryResult result = twitter.search(query);
    for (Status status : result.getTweets()) {
        System.out.println("@" + status.getUser().getScreenName() + ":" + status.getText());
    }
} catch (TwitterException e) {
println("Search tweets: " + e);
}
 
}
