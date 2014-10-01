//
//  ViewController.m
//  Underarm Thermometer
//
//  Created by Ryan Dougherty on 9/28/14.
//  Copyright (c) 2014 team3. All rights reserved.
//

// Assumes data comming in from the arduino is in degrees C, and is in a JSON 3 tuple
// Uses a specific URL on the host Wi-Fi that the arduino broadcasts to


#import "ViewController.h"

#define refreshRate 1.0f

@interface ViewController ()
    @property (strong, nonatomic) IBOutlet UILabel *instantaneousTemp;
    @property (strong, nonatomic) IBOutlet UILabel *oneSecTemp;
    @property (strong, nonatomic) IBOutlet UILabel *tenSecTemp;

    @property (assign) float oneSecData;
    @property (assign) float tenSecData;
    @property (assign) float instData;

    @property (strong, nonatomic) IBOutlet UILabel *unitLabel;
@end

@implementation ViewController
    bool isAlarmOn = false;
    bool didGetPackage = true;

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    // Fake loading data
    self.instData = ([self.instantaneousTemp.text floatValue]-32)*5/9;
    self.oneSecData = ([self.oneSecTemp.text floatValue]-32)*5/9;
    self.tenSecData = ([self.tenSecTemp.text floatValue]-32)*5/9;
    self.isCelsius = true;
    
    // TODO Set the URL to the correct IP
    self.myURL = @"http://10.3.13.159";
    
    [self updateScreen];
    
    // Set Screen to Refresh according to the refresh rate
    [NSTimer scheduledTimerWithTimeInterval:refreshRate target:self selector:@selector(refreshScreen:) userInfo:nil repeats:YES];
    
    [self serverRequest];
    // TODO: Make update timer for grabbing Wi-Fi information
    // TODO: Set up arduino to broadcast to a URL, then grab the JSON from that location by setting myURL to the URL
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
}

// Called by the screen refresh timer, calls updateScreen
-(void)refreshScreen:(NSTimer *)timer{
    [self updateScreen];
}

// Update the screen with new information
-(void)updateScreen{
    [self serverRequest];
    
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
    }else{
        self.instantaneousTemp.text = @"--.-";
        self.oneSecTemp.text = @"--.-";
        self.tenSecTemp.text = @"--.-";
    }
    
    if(!isAlarmOn){
        [self checkHeat];
    }
    
    //NSLog(@"Screen Updated");
}


// Make request to server
-(void)serverRequest{
    //NSLog(@"Server Request");
    NSURL *url = [NSURL URLWithString:self.myURL];
    if(!url){
        NSLog(@"OH NO! THERE'S NO URL!");
        return;
    }
    
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),^{
        
        NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:url];
        [request setCachePolicy:NSURLRequestReloadIgnoringLocalAndRemoteCacheData];
        [request setValue:@"application/json; charset=utf-8" forHTTPHeaderField:@"Content-Type"];
        [request setValue:@"application/json" forHTTPHeaderField:@"Accept"];
        
        NSURLResponse *response;
        NSError *error;
        NSData *data = [NSURLConnection sendSynchronousRequest:request returningResponse:&response error:&error];
        if(!data){
            NSLog(@"AAAHHHH NO DATA!!!!");
            didGetPackage = false;
            return;
        }
        didGetPackage = true;
        NSLog(@"%@", data);
        [self performSelectorOnMainThread:@selector(handleResponse:) withObject:data waitUntilDone:YES];
    });
}

// Handle server response
-(void)handleResponse:(NSData *)response{
    
    NSLog(@"IM HERE");
    if(response){
        NSError *error;
        
        NSDictionary *jsonPackage = [NSJSONSerialization JSONObjectWithData:response options:kNilOptions error:&error];
        NSArray *arduinoData = jsonPackage[@"packet"];
        
        NSLog(@"%@", jsonPackage);
        NSLog(@"WHATTTTT????");
        self.instData = [[arduinoData valueForKey:@"last_reading"] floatValue];
        self.oneSecData = [[arduinoData valueForKey:@"s_avg"] floatValue];
        self.tenSecData = [[arduinoData valueForKey:@"tens_avg"] floatValue];
        
    }else{
        NSLog(@"ERROR OBTAINING RESULTS, CHECK URL");
    }
}

-(void)checkHeat{
    if(self.tenSecData>=32.2f || self.oneSecData>=32.2f || self.instData>=32.2){
        isAlarmOn = true;
        
        UIAlertView *theAlert = [[UIAlertView alloc] initWithTitle:@"Thermometer in Use"
            message:@"Your Device is Working"
            delegate:self
            cancelButtonTitle:@"Thank You"
            otherButtonTitles:nil];
        
        [theAlert show];
    }
}

@end
