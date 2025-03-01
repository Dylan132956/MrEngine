//
//  HomeViewController.m
//  Viry3DApp
//
//  Created by boob on 2024/3/25.
//

#import "HomeViewController.h"
#import "ViewController.h"
@interface HomeViewController ()
@property (retain, nonatomic) IBOutlet UIButton *btnnext;

@end

@implementation HomeViewController


-(UIButton *)btnnext{
    if (!_btnnext) {
        _btnnext = [UIButton buttonWithType:UIButtonTypeSystem];
        [_btnnext setTitle:@"show demo" forState:UIControlStateNormal];
        [_btnnext addTarget:self action:@selector(showNext:) forControlEvents:UIControlEventTouchUpInside];
        _btnnext.frame = CGRectMake(100, 100, 100, 50);
        [self.view addSubview:_btnnext];    
    }
    return _btnnext;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = [UIColor whiteColor];
    [self btnnext];
    // Do any additional setup after loading the view from its nib.
}
- (IBAction)showNext:(id)sender {
    
    [self.navigationController pushViewController:[[[ViewController new] init] autorelease] animated:YES];
}

/*
#pragma mark - Navigation

// In a storyboard-based application, you will often want to do a little preparation before navigation
- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender {
    // Get the new view controller using [segue destinationViewController].
    // Pass the selected object to the new view controller.
}
*/

- (void)dealloc {
    [_btnnext release];
    [super dealloc];
}
@end
