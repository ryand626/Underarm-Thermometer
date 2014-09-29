//
//  ViewController.h
//  Underarm Thermometer
//
//  Created by Ryan Dougherty on 9/28/14.
//  Copyright (c) 2014 team3. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface ViewController : UIViewController

@property (strong, nonatomic) NSString* myURL;
@property (assign, nonatomic) BOOL isCelsius;


- (IBAction)tempSwitch:(id)sender;

@end
