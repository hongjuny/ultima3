//
//  LWIntegerTransformer.m
//

#import "LWIntegerTransformer.h"


@implementation LWIntegerTransformer

+ (Class) transformedValueClass {
    return [NSNumber class];
}

+ (BOOL) allowsReverseTransformation {
    return NO;
}

- (id) transformedValue:(id)value {
	return [NSNumber numberWithInt:(int)roundf([value floatValue])];
}

@end
