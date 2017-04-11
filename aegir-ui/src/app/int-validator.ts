import { ValidatorFn, AbstractControl } from '@angular/forms';

export class IntValidator {

    constructor() { }

    static minValue(value: number): ValidatorFn {
	return (control: AbstractControl): {[key: string]: any} => {
	    if ( typeof control.value !== 'number' ) {
		return {'typeMismatch': 'not a number'}
	    }
	    if ( control.value < value ) {
		return {'valueMismatch': 'too low'}
	    }
	    return null;
	};
    }

    static maxValue(value: number): ValidatorFn {
	return (control: AbstractControl): {[key: string]: any} => {
	    if ( typeof control.value !== 'number' ) {
		return {'typeMismatch': 'not a number'}
	    }
	    if ( control.value > value ) {
		return {'valueMismatch': 'too high'}
	    }
	    return null;
	};
    }


}
