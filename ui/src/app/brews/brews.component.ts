import { Component, OnInit, OnDestroy } from '@angular/core';
import { ActivatedRoute, Params, Router } from '@angular/router';
import { switchMap } from 'rxjs/operators';
import { Validators, FormGroup, FormArray, FormControl,
	 FormBuilder, AbstractControl, ValidatorFn,
	 ValidationErrors } from '@angular/forms';

import { faFileCirclePlus, faPaperPlane } from '@fortawesome/free-solid-svg-icons';

import { ApiService } from '../api.service';
import { apiBrew, apiYeast, apiFermenter, apiTransfer } from '../api.types';

import { sg2brix, brix2sg } from '../units';

interface brew {
  errors: string[],
  data: apiBrew,
  transferMode: boolean,
  transferForm: FormGroup,
};

@Component({
  selector: 'app-brews',
  templateUrl: './brews.component.html',
  styleUrls: ['./brews.component.css']
})
export class BrewsComponent {
  faFileCirclePlus = faFileCirclePlus;
  faPaperPlane = faPaperPlane;

  private fermdid: number|null = null;
  public showNew: boolean = false;
  public yeasts: apiYeast[] = [];
  public fermenters: apiFermenter[] = [];
  public isopen_active: boolean = true;
  public isopen_finished: boolean = false;
  public brews: brew[] = [];

  public addBrewForm: FormGroup = new FormGroup({
    name: new FormControl("", [Validators.required,
			       Validators.minLength(3)]),
    yeastid: new FormControl(null, [Validators.required]),
    hasoriginalsg: new FormControl(false, []),
    originalsg: new FormControl(1.040, [Validators.required]),
    originalsgunits: new FormControl('sg', []),
    sgoffset: new FormControl(0, [Validators.required,
				  Validators.min(0), Validators.max(10)]),
    sgoffsetunits: new FormControl('brix', []),
  });

  constructor(private api: ApiService,
    private route: ActivatedRoute,
    private router: Router) {
  }

  ngOnInit() {
    this.brews = [];
    this.yeasts = [];
    this.route.parent!.params
      .subscribe(
	(params:Params) => {
	  this.fermdid = params['fermdid'];
	  this.updateBrews();
	  this.updateYeasts();
	  this.updateFermenters();
	}
      );

    // listen to addform changes
    this.addBrewForm._old = this.addBrewForm.value;
    this.formChanges(this.addBrewForm);
  }

  formChanges(model: FormGroup) {
    model.valueChanges.subscribe(
      (value:any) => {
	var old = model._old;
	// on hasoriginalsg changes
	if ( old.hasoriginalsg! && !value.hasoriginalsg ) {
	  // originalsg being disabled
	  model.get('originalsg')!.clearValidators();
	  model.get('originalsg')!.updateValueAndValidity({emitEvent: false});
	  model.updateValueAndValidity({emitEvent: false});
	} else if ( !old.hasoriginalsg! && value.hasoriginalsg ||
	  value.hasoriginalsg && old.originalsgunits! != value.originalsgunits ) {
	  // originalsg being enabled
	  if ( value.originalsgunits == "brix" ) {
	    model.get('originalsg')!.setValidators([Validators.required,
						    Validators.min(2),
						    Validators.max(20)]);
	  } else {
	    // sg
	    model.get('originalsg')!.setValidators([Validators.required,
						    Validators.min(1.010),
						    Validators.max(1.100)]);
	  }
	  model.get('originalsg')!.updateValueAndValidity({emitEvent: false});
	  model.updateValueAndValidity({emitEvent: false});
	}

	// if the originalsg units have changed
	if ( value.hasoriginalsg
	  && old.originalsgunits! != value.originalsgunits) {
	  let ctrl = model.get('originalsg')!
	  let currval = ctrl.value;
	  if ( value.originalsgunits == 'brix' ) {
	    let newval = sg2brix(currval);
	    newval = (Math.round(newval*100)/100);
	    ctrl.setValue(newval.toFixed(1), {emitEvent: false});
	  } else {
	    let newval = brix2sg(currval)
	    newval = (Math.round(newval*1000)/1000);
	    ctrl.setValue(newval.toFixed(3), {emitEvent: false});
	  }
	}

	// sgoffset changes
	if ( old.sgoffsetunits! != value.sgoffsetunits) {
	  let ctrl = model.get('sgoffset')!
	  let currval = ctrl.value;
	  if ( value.sgoffsetunits == 'brix' ) {
	    let newval = sg2brix(currval);
	    newval = (Math.round(newval*100)/100);
	    ctrl.setValidators([Validators.required,
				Validators.min(0),
				Validators.max(10)]);
	    ctrl.setValue(newval.toFixed(1), {emitEvent: false});
	  } else {
	    let newval = brix2sg(currval)
	    newval = (Math.round(newval*1000)/1000);
	    ctrl.setValidators([Validators.required,
				Validators.min(1.0),
				Validators.max(1.040)]);
	    ctrl.setValue(newval.toFixed(3), {emitEvent: false});
	  }
	  model.updateValueAndValidity({emitEvent: false});
	}

	model._old = value;
      }
    );
  }

  updateBrews() {
    this.api.getBrews(this.fermdid!).subscribe(
      (data:apiBrew[]) => {
	let bs: brew[] = [];
	for (let b of data) {
	  bs.push({errors: [],
		   data: b,
		   transferMode: false,
		   transferForm: this.createTransferFormGroup(b)})
	}
	this.brews = bs;
	//console.log('Loaded brews', this.brews);
      }
    );
  }

  createTransferFormGroup(b: apiBrew): FormGroup {
    let fid:number|null = null;
    if ( b.fermenter ) fid = b.fermenter!.id!;
    return new FormGroup({
      fermenterid: new FormControl(fid, [Validators.required]),
    });
  }

  updateYeasts() {
    this.api.getYeasts(this.fermdid!).subscribe(
      (data:apiYeast[]) => {
	this.yeasts = data;
      }
    );
  }

  updateFermenters() {
    this.api.getFermenters(this.fermdid!).subscribe(
      (data:apiFermenter[]) => {
	this.fermenters = data;
      }
    );
  }

  getEmptyFermenters(): apiFermenter[] {
    let fs: apiFermenter[] = [];

    for (let f of this.fermenters) {
      if ( !f.brew ) {
	fs.push(f);
      }
    }

    return fs;
  }

  add(model: FormGroup) {
    let yeast: apiYeast|null = null;
    let yid = model.get('yeastid')!.value;
    for (let y of this.yeasts) {
      if ( y.id! == yid ) {
	yeast = y;
	break;
      }
    }
    let brew: apiBrew = {
      name: model.get('name')!.value,
      yeast: yeast!,
      brewdate: 'null',
      sgoffset: model.get('sgoffset')!.value,
      finished: false
    };

    if ( model.get('hasoriginalsg')!.value ) {
      let sgval = model.get('originalsg')!.value;
      let sgunit = model.get('originalsgunits')!.value;
      if ( sgunit == 'brix' ) {
	brew.originalsg = brix2sg(sgval);
      } else {
	brew.originalsg = sgval;
      }
    }
    this.api.addBrew(this.fermdid!, brew).subscribe(
      (data: any) => {
	this.showNew = false;
	this.updateBrews();
      }
    );
  }

  transfer(b: brew) {
    let f: apiFermenter|null = null;
    for (let it of this.fermenters) {
      if ( it.id! == (b.transferForm.get('fermenterid')!.value as number) ) {
	f = it;
	break;
      }
    }
    let transfer: apiTransfer = {
      brew: b.data,
      fermenter: f as apiFermenter,
    };
    b.transferMode = false;
    this.api.transferBrew(this.fermdid!, transfer).subscribe(
      (data: any) => {
	this.updateBrews();
      }
    );
  }

  hasFinishedBrews(): boolean {
    for (let b of this.brews) {
      if ( b.data.finished ) return true;
    }
    return false;
  }

  hasActiveBrews(): boolean {
    for (let b of this.brews) {
      if ( !b.data.finished ) return true;
    }
    return false;
  }

  activeBrews(): brew[] {
    let ret: brew[] = [];

    for (let b of this.brews) {
      if ( !b.data.finished )
	ret.push(b);
    }

    return ret;
  }

  finishedBrews(): brew[] {
    let ret: brew[] = [];

    for (let b of this.brews) {
      if ( b.data.finished )
	ret.push(b);
    }

    return ret;
  }

  addFormHasOriginalSG(): boolean {
    return this.addBrewForm.get('hasoriginalsg')!.value;
  }

  toggleAddFormHasOriginalSG() {
    this.addBrewForm.get('hasoriginalsg')!.setValue(!this.addBrewForm.get('hasoriginalsg')!.value);
  }
}
