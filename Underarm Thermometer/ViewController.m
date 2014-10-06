//
//  ViewController.m
//  Underarm Thermometer
//
//  Created by Ryan Dougherty on 9/28/14.
//  Copyright (c) 2014 team3. All rights reserved.
//

// Assumes data comming in from the arduino is in degrees C, and is in a JSON 3 tuple
// Uses a specific URL on the host Wi-Fi that the arduino broadcasts to

// Audio code based off of the tutorial at: http://tech.pro/tutorial/973/create-a-basic-iphone-audio-player-with-av-foundation-framework
// Wi-Fi code based off of the tutorial at: http://www.raywenderlich.com/38841/arduino-tutorial-temperature-sensor


#import "ViewController.h"

#define refreshRate 1.25f
#define slope 0.0614f
#define offset -5.6311f

@interface ViewController ()
    // Temperature Labels
    @property (strong, nonatomic) IBOutlet UILabel *instantaneousTemp;
    @property (strong, nonatomic) IBOutlet UILabel *oneSecTemp;
    @property (strong, nonatomic) IBOutlet UILabel *tenSecTemp;

    // Temperature Data
    @property (assign) float oneSecData;
    @property (assign) float tenSecData;
    @property (assign) float instData;

    @property (assign) NSURL* url;

    // Unit Conversion Label
    @property (strong, nonatomic) IBOutlet UILabel *unitLabel;

    // Audio
    @property (assign) SystemSoundID alarmSound;
    
@end

@implementation ViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    isAlarmOn = true;
    didGetPackage = false;
    isConnected = false;
    
    alarmTimer = 0;

    self.isCelsius = true;
    
    // Set the URL to the correct IP
    self.myURL = @"http://10.3.13.158";
    
    // Load the Audio Files
    [self loadSoundFiles];
    [self updateScreen];
    
    // Set Screen to Refresh according to the refresh rate
    [NSTimer scheduledTimerWithTimeInterval:refreshRate target:self selector:@selector(refreshScreen:) userInfo:nil repeats:YES];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

// The user switches which temperature unit they want to see
- (IBAction)tempSwitch:(id)sender {
    UISegmentedControl* tempSwitch = sender;
    int tempState = (int)[tempSwitch selectedSegmentIndex];
    if(tempState == 0){
        self.unitLabel.text = @"Celsius";
        self.isCelsius = true;
    }else{
        self.unitLabel.text = @"Fahrenheit";
        self.isCelsius = false;
    }
    
    [self updateScreen];
    [effectPlayer play];
}

// Called by the screen refresh timer, calls updateScreen
-(void)refreshScreen:(NSTimer *)timer{
    [self serverRequest];
    [self updateScreen];
}

// Update the screen with new information
-(void)updateScreen{
    self.instantaneousTemp.text = @"--.-";
    self.oneSecTemp.text = @"--.-";
    self.tenSecTemp.text = @"--.-";
    if(isConnected){
        if(didGetPackage){
            // Check to se if temperature is in Celcius or Fahrinheit, display accordingly
            if(self.isCelsius){
                self.instantaneousTemp.text = [NSString stringWithFormat:@"%.1f",self.instData];
                self.oneSecTemp.text = [NSString stringWithFormat:@"%.1f",self.oneSecData];
                self.tenSecTemp.text = [NSString stringWithFormat:@"%.1f",self.tenSecData];
            }else{
                self.instantaneousTemp.text = [NSString stringWithFormat:@"%.1f", self.instData*9/5+32];
                self.oneSecTemp.text = [NSString stringWithFormat:@"%.1f",self.oneSecData*9/5+32];
                self.tenSecTemp.text = [NSString stringWithFormat:@"%.1f",self.tenSecData*9/5+32];
                
            }
        }
    }
    
    [self checkHeat];
    
    alarmTimer +=1;
}


// Make request to server
-(void)serverRequest{
    //NSLog(@"Requesting Server");
    // Load URL
    NSURL *url = [NSURL URLWithString:self.myURL];
    if(!url){
        NSLog(@"NO URL");
        isConnected = false;
        return;
    }
    isConnected = true;

    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),^{
        
        NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:url];
        [request setCachePolicy:NSURLRequestReloadIgnoringLocalAndRemoteCacheData];
        [request setValue:@"application/json; charset=utf-8" forHTTPHeaderField:@"Content-Type"];
        [request setValue:@"application/json" forHTTPHeaderField:@"Accept"];
        
        NSURLResponse *response;
        NSError *error;
        NSData *data = [NSURLConnection sendSynchronousRequest:request returningResponse:&response error:&error];
        
        if(!data){
            NSLog(@"NO DATA");
            NSLog(@"%@",[error localizedDescription]);
            didGetPackage = false;
            return;
        }
        didGetPackage = true; // If data was obtained, set the flag to true
        // NSLog(@"%@", data);
        [self performSelectorOnMainThread:@selector(handleResponse:) withObject:data waitUntilDone:YES];
    });
}

// Handle server response
-(void)handleResponse:(NSData *)response{
    if(response){
        NSError *error;
        
        NSDictionary *jsonPackage = [NSJSONSerialization JSONObjectWithData:response options:kNilOptions error:&error];
        NSArray *arduinoData = jsonPackage[@"packet"];
        
        NSLog(@"%@", jsonPackage);
        
        self.instData = [self convert:[[arduinoData valueForKey:@"last_reading"] floatValue]];
        self.oneSecData = [self convert:[[arduinoData valueForKey:@"s_avg"] floatValue]];
        self.tenSecData = [self convert:[[arduinoData valueForKey:@"tens_avg"] floatValue]];
        
    }else{
        NSLog(@"ERROR OBTAINING RESULTS, CHECK URL");
    }
}

-(void)checkHeat{
    if(self.instData >= 32.2){
        if(isAlarmOn){
            UIAlertView *tempAlert = [[UIAlertView alloc] initWithTitle:@"High Temperatrue"
                                                                message:@"Your Temperature is above 90"
                                                               delegate:self
                                                      cancelButtonTitle:@"Thank You"
                                                      otherButtonTitles:nil];
            [tempAlert setTag:1];
            alarmTimer = 0;
            [tempAlert show];
            [self playAlarm];
        }
        isAlarmOn = false;
    }else{
        isAlarmOn = true;
    }
}

-(void)loadSoundFiles{
    // Set up file path to audio file
    NSString* path = [[NSBundle mainBundle] pathForResource:@"sms-received2" ofType:@"mp3"];
    NSURL* file = [NSURL fileURLWithPath:path];
    
    // Set up audio player
    alarmPlayer = [[AVAudioPlayer alloc] initWithContentsOfURL:file error:nil];
    [alarmPlayer prepareToPlay];
    
    path = [[NSBundle mainBundle] pathForResource:@"Tink" ofType:@"mp3"];
    file = [NSURL fileURLWithPath:path];
    
    // Set up audio player
    effectPlayer = [[AVAudioPlayer alloc] initWithContentsOfURL:file error:nil];
    [effectPlayer prepareToPlay];
    
    NSLog(@"Sound files loaded");
}

-(void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex{
    if(alertView.tag == 1){
        if(buttonIndex == 0){
           // isAlarmOn = false;
        }
    }
}

- (void) playAlarm {
    NSLog(@"Playing Sound");
    if ([alarmPlayer isPlaying]) {
        [alarmPlayer pause];
    } else {
        [alarmPlayer play];
    }
}

-(float)convert:(float)degree{
    return degree * slope + offset;
}

@end
