//
//  ViewController.h
//  Underarm Thermometer
//
//  Created by Ryan Dougherty on 9/28/14.
//  Copyright (c) 2014 team3. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AVFoundation/AVFoundation.h>

@interface ViewController : UIViewController<UIAlertViewDelegate>{
    bool isAlarmOn;
    bool didGetPackage;
    bool isConnected;
    int alarmTimer;

    AVAudioPlayer* audioPlayer;
    AVAudioPlayer* effectPlayer;
    AVAudioPlayer* alarmPlayer;
}

@property (strong, nonatomic) NSString* myURL;
@property (assign, nonatomic) BOOL isCelsius;


- (IBAction)tempSwitch:(id)sender;

@end
