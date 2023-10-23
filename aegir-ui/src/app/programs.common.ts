import { Validators, FormGroup, FormArray, FormControl,
	 FormBuilder, AbstractControl, ValidationErrors } from '@angular/forms';

import { apiProgram } from './api.types';

export interface programForm {
  minmashtemp:number;
  maxmashtemp:number;
}

export function initMashStep(n: number, fb: FormBuilder, endtemp: number) {
  return fb.group({
    order: [n],
    temp: [42, [Validators.required,
		(control: AbstractControl) => Validators.min(20)(control),
		(control: AbstractControl) => Validators.max(endtemp)(control)]],
    holdtime: [15, [Validators.required]]
  });
}

export function initHop(form: FormGroup, fb: FormBuilder) {
  const boiltime = form.get('boiltime')!.value;
  return fb.group({
    attime: [0, [Validators.required,
		 (control: AbstractControl) => Validators.min(0)(control),
		 (control: AbstractControl) => Validators.max(boiltime)(control)]],
    name: ['', [Validators.required,
		Validators.minLength(3)]],
    quantity: [0, [Validators.required,
		   (control: AbstractControl) => Validators.min(1)(control)]]
  });
}

export function formValidator(form: AbstractControl): ValidationErrors | null {
  //console.log('formValidator', form)

  let startval = form.get('starttemp')!.value;
  let endval = form.get('endtemp')!.value;

  const msctrl : FormArray = form.get('mashsteps') as FormArray;
  let tempsteps: Array<number> = msctrl.value;

  let absmin = startval;
  let absmax = endval;
  let i = 0;
  for ( let step of msctrl['controls'] ) {
    let min = <number>(i==0 ? absmin : tempsteps[i-1]);
    let max = <number>(i+1==msctrl['controls']!.length ? absmax : tempsteps[i+1]);
    let stepval = step.get('temp')!.value;
    /*
      console.log('i:', i, ' Min:', min, ' max:', max, ' val:', stepval,
      ' status:', step.get('temp').status);
    */
    step.get('temp')!.setValidators([Validators.required,
				     (control: AbstractControl) => Validators.min(min+1)(control),
				     (control: AbstractControl) => Validators.max(max-1)(control)]);
    if ( (stepval <= min || stepval >= max) || step.get('temp')!.invalid ) {
      step.get('temp')!.markAsTouched();
    }
    if ( (stepval > min && stepval < max && step.get('temp')!.invalid) ||
      (stepval <= min || stepval >= max) && step.get('temp')!.valid ) {
      step.get('temp')!.updateValueAndValidity();
    }
    i += 1;
  }

  let hasboil = form.get('hasboil')!.value;
  let hasmash = form.get('hasmash')!.value;
  //console.log('add-p::formvalidator', hasboil, hasmash);
  if ( !hasboil && !hasmash ) return {'mashboil': 'Either mash or boil is needed'};

  if ( hasboil ) {
    const hops : FormArray = form.get('hops') as FormArray;
    if ( hops.length == 0 )
      return {'needhops': 'Need hops when boiling'}
  }

  if ( hasmash ) {
    const steps = form.get('mashsteps')!.value;
    if ( steps.length == 0 )
      return {'needmashsteps': 'Need at least 1 mash steps'}
  }
  return null;
} // formValidator

export function defineValidators(form: FormGroup,
				 obj: programForm) {
  form.setValidators(formValidator);

  // Add the starttemp validator, which updates the mashtemp validators
  form.get('starttemp')!.valueChanges.subscribe({
    next: (value:number) => {
      const endctrl = form.get('endtemp') as FormArray;
      let endval = endctrl.value;

      // set the mintemp's max
      if ( value != obj.minmashtemp ) {
	endctrl.setValidators([Validators.required,
			       (control: AbstractControl) => Validators.min(value)(control),
			       (control: AbstractControl) => Validators.max(90)(control)]);
	endctrl.updateValueAndValidity();
	obj.minmashtemp = value;
      }
    }
  });

  // Add the endtemp validator, which updates the mashtemp validators
  form.get('endtemp')!.valueChanges.subscribe({
    next: (value:number) => {
      const startctrl = form.get('starttemp') as FormArray;
      let startval = startctrl.value;

      // set the mintemp's max
      if ( value != obj.maxmashtemp ) {
	startctrl.setValidators([Validators.required,
				 (control: AbstractControl) => Validators.min(25)(control),
				 (control: AbstractControl) => Validators.max(value)(control)]);
	startctrl.updateValueAndValidity();
	obj.maxmashtemp = value;
      }
    }
  });

  // Add the boiltime validator, which updates the hop timing validators
  form.get('boiltime')!.valueChanges.subscribe({
    next: (value:number) => {
      const hopctrl = form.get('hops') as FormArray;

      for ( let hop of hopctrl.controls ) {
	hop!.get('attime')!.setValidators([Validators.required,
					   (control: AbstractControl) => Validators.min(0)(control),
					   (control: AbstractControl) => Validators.max(value)(control)]);
	hop!.get('attime')!.updateValueAndValidity();
      }
    }
  });
}

export function toApiProgram(model: FormGroup): apiProgram {
  let hasmash = model.get('hasmash')!.value;
  let hasboil = model.get('hasboil')!.value;
  let p : apiProgram = {
    starttemp: model.get('starttemp')!.value,
    endtemp: model.get('endtemp')!.value,
    boiltime: model.get('boiltime')!.value,
    name: model.get('name')!.value,
    nomash: !hasmash,
    noboil: !hasboil,
    mashsteps: model.get('mashsteps')!.value,
    hops: model.get('hops')!.value
  };

  // only save has ID, addprogram does not yet
  let progid = model.get('id');
  if ( progid ) p.id = progid.value;

  return p
}
