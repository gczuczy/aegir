import { Component, OnInit } from '@angular/core';
import { Validators, FormGroup, FormArray, FormControl,
	 FormBuilder, AbstractControl, ValidatorFn, ValidationErrors } from '@angular/forms';
import { Router } from '@angular/router';

import { ApiService } from '../api.service';
import { apiAddProgramData,
	 apiProgram, apiProgramHopStep, apiProgramMashStep } from '../api.types';

@Component({
  selector: 'app-add-program',
  templateUrl: './add-program.component.html',
  styleUrls: ['./add-program.component.css']
})
export class AddProgramComponent implements OnInit {

  errors: string[] | null = null;

  private minmashtemp:number = 37;
  private maxmashtemp:number = 85;

  public hasmash:boolean = true;
  public hasboil:boolean = true;

  addProgramForm: FormGroup = new FormGroup({
    name: new FormControl("", [Validators.required,
			       Validators.minLength(3)]),
    starttemp: new FormControl(this.minmashtemp, [Validators.required,
						  (control: AbstractControl) => Validators.min(25)(control),
						  (control: AbstractControl) => Validators.max(85)(control)]),
    endtemp: new FormControl(this.maxmashtemp, [Validators.required,
						(control: AbstractControl) => Validators.min(37)(control),
						(control: AbstractControl) => Validators.max(90)(control)]),
    boiltime: new FormControl(60, [Validators.required,
				   (control: AbstractControl) => Validators.min(30)(control),
				   (control: AbstractControl) => Validators.max(300)(control)]),
    hasmash: new FormControl(this.hasmash, []),
    mashsteps: this.fb.array([this.initMashStep(0)]),
    hasboil: new FormControl(this.hasboil, []),
    hops: this.fb.array([])
  });

  constructor(private api: ApiService,
	      private fb: FormBuilder,
	      private router: Router) {
  }

  ngOnInit() {
    this.errors = null;

    this.hasmash = true;
    this.hasboil = true;

    this.addProgramForm.setValidators(this.formValidator.bind(this));

    // Add the starttemp validator, which updates the mashtemp validators
    this.addProgramForm.controls['starttemp'].valueChanges.subscribe({
      next: (value:number) => {
	const endctrl = <FormArray>this.addProgramForm!.controls['endtemp'];
	let endval = endctrl.value

	// set the mintemp's max
	if ( value != this.minmashtemp ) {
	  endctrl.setValidators([Validators.required,
				 (control: AbstractControl) => Validators.min(value)(control),
				 (control: AbstractControl) => Validators.max(90)(control)]);
	  endctrl.updateValueAndValidity();
	  this.minmashtemp = value;
	}
      }
    });

    // Add the endtemp validator, which updates the mashtemp validators
    this.addProgramForm.controls['endtemp'].valueChanges.subscribe({
      next: (value:number) => {
	const startctrl = <FormArray>this.addProgramForm!.controls['starttemp'];
	let startval = startctrl.value

	// set the mintemp's max
	if ( value != this.maxmashtemp ) {
	  startctrl.setValidators([Validators.required,
				   (control: AbstractControl) => Validators.min(25)(control),
				   (control: AbstractControl) => Validators.max(value)(control)]);
	  startctrl.updateValueAndValidity();
	  this.maxmashtemp = value;
	}
      }
    });

    // Add the boiltime validator, which updates the hop timing validators
    this.addProgramForm.controls['boiltime'].valueChanges.subscribe({
      next: (value:number) => {
	const hopctrl = <FormArray>this.addProgramForm!.controls['hops'];

	for ( let hop of hopctrl.controls ) {
	  hop!.get('attime')!.setValidators([Validators.required,
					   (control: AbstractControl) => Validators.min(0)(control),
					   (control: AbstractControl) => Validators.max(value)(control)]);
	  hop!.get('attime')!.updateValueAndValidity();
	}
      }
    });

  } // ngOnInit

  public formValidator(form: AbstractControl): ValidationErrors | null {
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

  initMashStep(n: number) {
    let endtemp = 80;
    if ( this.addProgramForm ) {
      endtemp = this.addProgramForm.controls['endtemp'].value
    }
    return this.fb.group({
      order: [n],
      temp: [42, [Validators.required,
		  (control: AbstractControl) => Validators.min(20)(control),
		  (control: AbstractControl) => Validators.max(endtemp)(control)]],
      holdtime: [15, [Validators.required]]
    });
  }

  public addMashStep() {
    const control = <FormArray>this.addProgramForm.controls['mashsteps'];
    let ms = this.initMashStep(control.length);
    control.push(ms);
  }

  public removeMashStep(n: number) {
    const control = <FormArray>this.addProgramForm.controls['mashsteps'];
    control.removeAt(n)

    // and fix the order fields
    const msctrl = <FormArray>this.addProgramForm.controls['mashsteps'];

    let i = 0;
    for ( let step of msctrl.controls ) {
      step.get('order')!.setValue(i);
      i = i + 1;
    }
  }

  initHop() {
    const boiltime = this.addProgramForm.controls['boiltime']
    return this.fb.group({
      attime: [0, [Validators.required,
		  (control: AbstractControl) => Validators.min(0)(control),
		  (control: AbstractControl) => Validators.max(boiltime.value)(control)]],
      name: ['', [Validators.required,
		  Validators.minLength(3)]],
      quantity: [0, [Validators.required,
		     (control: AbstractControl) => Validators.min(1)(control)]]
    });
  }

  public addHop() {
    const control = <FormArray>this.addProgramForm.controls['hops'];
    control.push(this.initHop());
  }

  public removeHop(i: number) {
    const control = <FormArray>this.addProgramForm.controls['hops'];
    control.removeAt(i)
  }

  onHasmashChange(event:any) {
    this.hasmash = event['checked'];
  }

  onHasboilChange(event:any) {
    this.hasboil = event['checked'];
  }

  get mashSteps(): FormArray {
    return this.addProgramForm.get('mashsteps') as FormArray;
  }

  get hops(): FormArray {
    return this.addProgramForm.get('hops') as FormArray;
  }

  getHops(model: FormArray): apiProgramHopStep[] {
    return model.value;
  }

  getMashSteps(model: FormArray): apiProgramMashStep[] {
    return model.value;
  }

  public save(model: FormGroup) {
    //console.log('save ', model);
    let hasmash = model.get('hasmash')!.value;
    let hasboil = model.get('hasboil')!.value;
    let p : apiProgram = {
      starttemp: model.get('starttemp')!.value,
      endtemp: model.get('endtemp')!.value,
      boiltime: model.get('boiltime')!.value,
      name: model.get('name')!.value,
      nomash: !hasmash,
      noboil: !hasboil,
      mashsteps: this.getMashSteps(model.get('mashsteps')! as FormArray),
      hops: this.getHops(model.get('hops')! as FormArray)
    };
    this.api.addProgram(p).subscribe(
      (resp:apiAddProgramData) => {
	let progid = resp.progid;
	this.errors = null;
	this.api.announceUpdate();
	//console.log('Program Added with response: ', resp);
	this.router.navigate(['programs', progid, 'view']);
      },
      (err:any) => {
	let errobj = err.json();
	//console.log('Error obj: ', errobj);
	this.errors = errobj['errors'];
      }
    );
  }
}
