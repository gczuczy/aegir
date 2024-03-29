import { Component, OnInit, OnDestroy } from '@angular/core';
import { ActivatedRoute, Params, Router } from '@angular/router';
import { timer } from 'rxjs';
import { switchMap } from 'rxjs/operators';
import { Validators, FormGroup, FormArray, FormControl,
	 FormBuilder, AbstractControl, ValidatorFn,
	 ValidationErrors } from '@angular/forms';

import { ApiService } from '../api.service';
import { apiTilthydrometer, apiFermenter, apiSensorCache } from '../api.types';

interface tilthydrometer {
  errors: string[],
  data: apiTilthydrometer,
  cardcontent: string|null,
  assignForm: FormGroup,
  calibrateForm: FormGroup,
};

@Component({
  selector: 'app-tilthydrometers',
  templateUrl: './tilthydrometers.component.html',
  styleUrls: ['./tilthydrometers.component.css']
})
export class TilthydrometersComponent implements OnInit, OnDestroy {
  public isopen_assigned=true;
  public isopen_enabled=true;

  private fermdid: number|null = null;
  private timer : any;
  private timer_sub : any;

  private tilts: tilthydrometer[]|null = null;
  public fermenters: apiFermenter[] = [];

  constructor(private api: ApiService,
    private route: ActivatedRoute,
    private router: Router) {
    this.timer = timer(1000, 1000);
  }

  ngOnInit() {
    this.route.parent!.params
      .subscribe(
	(params:Params) => {
	  this.fermdid = params['fermdid'];
	  this.updateFermenters();
	  this.updateTilts();
	}
      );
    this.timer_sub = this.timer.subscribe(
      (t:any) => {
	if ( t%30 == -1 ) this.updateTilts();
	this.getSensorCache();
      }
    );
  }

  ngOnDestroy() {
    this.timer_sub.unsubscribe();
  }

  updateFermenters() {
    this.api.getFermenters(this.fermdid!).subscribe(
      (data:apiFermenter[]) => {
	this.fermenters = data;
      }
    );
  }

  updateTilts() {
    if ( this.fermdid == null ) return;
    this.api.getTilthydrometers(this.fermdid!).subscribe(
      (data:apiTilthydrometer[]) => {
	let ts: tilthydrometer[] = [];
	for ( let t of data ) {
	  let cardcontent = null;
	  if ( t.fermenter === "" ) {
	    t.fermenter = undefined;
	  } else {
	    cardcontent = 'assigned';
	  }
	  ts.push({'data': t,
		   'errors': [],
		   'cardcontent': cardcontent,
		   'assignForm': this.createAssignFormGroup(),
		   'calibrateForm': this.createCalibrateFormGroup(t)});
	}
	this.tilts = ts;
	//console.log('Updated tilts: ', this.tilts);
      }
    );
  }

  getSensorCache() {
    if ( this.fermdid == null ) return;
    if ( this.tilts == null ) return;

    let hasactive = false;
    for ( let t of this.tilts! ) {
      if ( t.cardcontent == null || t.cardcontent != 'calibrate' ) continue;

      if ( t.calibrateForm.get('autosg')!.value ) {
	hasactive = true;
	break;
      }
    }

    if ( !hasactive ) return;

    this.api.getSensorCache(this.fermdid!).subscribe(
      (data:apiSensorCache) => {
	//console.log('getSensorCache', data);
	for ( let sensor of data.tilthydrometers ) {
	  // look for the one by uuid
	  for ( let tilt of this.tilts! ) {
	    if ( tilt.data.uuid == sensor.uuid &&
	      tilt.cardcontent == 'calibrate' &&
	      tilt.calibrateForm.get('autosg')!.value ) {
	      tilt.calibrateForm.get('calibr_at')!.setValue(sensor.sg);
	    }
	  }
	}
      }
    );
  }

  isNull(data: any): boolean {
    let ret = data == null || data == "null" || data === ""
    //console.log('isNull', data, ret);
    return ret;
  }

  disableSpecSG(model: FormGroup) {
    if ( model.get('autosg')!.value ) {
      model.get('calibr_at')!.disable();
    } else {
      model.get('calibr_at')!.enable();
    }
  }

  createAssignFormGroup(): FormGroup {
    return new FormGroup({
      fermenterid: new FormControl('', [Validators.required]),
    });
  }

  createCalibrateFormGroup(data: apiTilthydrometer): FormGroup {
    let calibr_null = null;
    let calibr_sg = null;
    let calibr_at = null;
    let has_null = false;
    let has_sg = false;

    if ( !this.isNull(data.calibr_null) ) {
      calibr_null = data.calibr_null as number;
      has_null = true;
    }
    if ( !this.isNull(data.calibr_at) &&
      !this.isNull(data.calibr_sg) ) {
      calibr_at = data.calibr_at as number;
      calibr_sg = data.calibr_sg as number;
      has_sg = true;
    }

    return new FormGroup({
      nullcalibr: new FormControl(has_null, []),
      sgcalibr: new FormControl(has_sg, []),
      autosg: new FormControl('', []),
      calibr_null: new FormControl(calibr_null, [Validators.required,
						 Validators.min(1.000),
						 Validators.max(1.015)]),
      calibr_at: new FormControl(calibr_at, [Validators.required,
					     Validators.min(1.000),
					     Validators.max(1.200)]),
      calibr_sg: new FormControl(calibr_sg, [Validators.required,
					     Validators.min(1.000),
					     Validators.max(1.200)]),
    });
  }

  assignedTilts(): tilthydrometer[] {
    if ( this.tilts == null ) return [];

    let ts: tilthydrometer[] = [];

    for (let t of this.tilts) {
      if ( t.data.fermenter != undefined ) {
	ts.push(t);
      }
    }
    return ts;
  }

  enabledTilts(): tilthydrometer[] {
    if ( this.tilts == null ) return [];

    let ts: tilthydrometer[] = [];

    for (let t of this.tilts) {
      if ( t.data.enabled && t.data.fermenter == undefined ) {
	ts.push(t);
      }
    }
    return ts;
  }

  disabledTilts(): tilthydrometer[] {
    if ( this.tilts == null ) return [];

    let ts: tilthydrometer[] = [];

    for (let t of this.tilts) {
      if ( !t.data.enabled ) {
	ts.push(t);
      }
    }
    return ts;
  }

  enableTilt(tiltid: number) {
    let data = new Map<string, string|number|boolean>();
    data.set('enabled', true);
    this.api.updateTilthydrometer(this.fermdid!,
				  tiltid, data)
      .subscribe(
	(data:apiTilthydrometer) => {
	  this.updateTilts();
	}
      );
  }

  disableTilt(tiltid: number) {
    let data = new Map<string, string|number|boolean>();
    data.set('enabled', false);
    this.api.updateTilthydrometer(this.fermdid!,
				  tiltid, data)
      .subscribe(
	(data:apiTilthydrometer) => {
	  this.updateTilts();
	}
      );
  }

  assign(tiltid: number, model: FormGroup) {
    let data = new Map<string, any>();
    data.set('fermenter', {'id': model.get('fermenterid')!.value});
    this.api.updateTilthydrometer(this.fermdid!,
				  tiltid, data)
      .subscribe(
	(data:apiTilthydrometer) => {
	  this.updateTilts();
	},
	(err:any) => {
	  for (let t of this.tilts!) {
	    if ( t.data.id == tiltid ) {
	      t.cardcontent = null;
	      break;
	    }
	  }
	}
      );
  }

  unassign(tiltid: number) {
    //console.log('unassign', tiltid);
    let data = new Map<string, any>();
    data.set('fermenter', null);
    this.api.updateTilthydrometer(this.fermdid!,
				  tiltid, data)
      .subscribe(
	(data:apiTilthydrometer) => {
	  this.updateTilts();
	},
	(err:any) => {
	  for (let t of this.tilts!) {
	    if ( t.data.id == tiltid ) {
	      t.cardcontent = null;
	      break;
	    }
	  }
	}
      );
  }

  isCalibrFromValid(model: FormGroup): boolean {
    let validNull = true;
    let validSg = true;

    if ( model.get('nullcalibr')!.value ) {
      validNull = model.get('calibr_null')!.valid;
    }

    if ( model.get('sgcalibr')!.value ) {
      validSg = (model.get('calibr_at')!.valid ||
	model.get('autosg')!.value ) &&
	model.get('calibr_sg')!.valid;
    }

    return validNull && validSg;
  };

  calibrate(tiltid: number, model: FormGroup) {
    //console.log('calibrate()', tiltid, model);
    let data = new Map<string, any>();

    if ( model.get('nullcalibr')!.value ) {
      data.set('calibr_null', model.get('calibr_null')!.value as number);
    } else {
      data.set('calibr_null', null);
    }

    if ( model.get('sgcalibr')!.value ) {
      data.set('calibr_at', model.get('calibr_at')!.value as number);
      data.set('calibr_sg', model.get('calibr_sg')!.value as number);
    } else {
      data.set('calibr_at', null);
      data.set('calibr_sg', null);
    }

    //console.log('Setting calibration', data);

    this.api.updateTilthydrometer(this.fermdid!,
				  tiltid, data)
      .subscribe(
	(data:apiTilthydrometer) => {
	  this.updateTilts();
	  for (let t of this.tilts!) {
	    if ( t.data.id == tiltid ) {
	      t.cardcontent = null;
	      t.errors = [];
	      break;
	    }
	  }
	},
	(err:any) => {
	  for (let t of this.tilts!) {
	    if ( t.data.id == tiltid ) {
	      //console.log(err);
	      t.errors = [err.error.message!];
	      break;
	    }
	  }
	}
      );
  }

}
