import { Component, OnInit } from '@angular/core';
import { ActivatedRoute, Params, Router } from '@angular/router';
import { timer } from 'rxjs';
import { switchMap } from 'rxjs/operators';
import { Validators, FormGroup, FormArray, FormControl,
	 FormBuilder, AbstractControl, ValidatorFn,
	 ValidationErrors } from '@angular/forms';

import { faFileCirclePlus, } from '@fortawesome/free-solid-svg-icons';

import { ApiService } from '../api.service';
import { apiFermenterType, apiFermenter } from '../api.types';

interface fermenter {
  data: apiFermenter,
  editForm: FormGroup,
  edit: boolean,
};

@Component({
  selector: 'app-fermenters',
  templateUrl: './fermenters.component.html',
  styleUrls: ['./fermenters.component.css']
})
export class FermentersComponent {
  faFileCirclePlus = faFileCirclePlus;

  fermdid: number|null = null;
  public showNew: boolean = false;
  public fermentertypes: apiFermenterType[] = [];
  public fermenters: fermenter[] = [];

  public addFermenterForm: FormGroup = new FormGroup({
    name: new FormControl("", [Validators.required,
			       Validators.minLength(3)]),
    ftid: new FormControl(42, [Validators.required]),
  });

  constructor(private api: ApiService,
	      private route: ActivatedRoute,
	      private router: Router,
	      private fb: FormBuilder) {
  }

  ngOnInit() {
    this.route.parent!.params
      .subscribe(
	(params:Params) => {
	  this.fermdid = params['fermdid'];
	}
      );
    this.updateFermenterTypes();
    this.updateFermenters();
  }

  updateFermenterTypes() {
    console.log("updateFermenterTypes(), fermdid:", this.fermdid);
    if ( this.fermdid == null ) return;
    this.api.getFermenterTypes(this.fermdid!).subscribe(
      (data:apiFermenterType[]) => {
	let fts: apiFermenterType[] = [];
	for (let ft of data) {
	  fts.push(ft);
	}
	this.fermentertypes = fts;
      }
    );
  }

  updateFermenters() {
    console.log("updateFermenterTypes(), fermdid:", this.fermdid);
    if ( this.fermdid == null ) return;
    this.api.getFermenters(this.fermdid!).subscribe(
      (data:apiFermenter[]) => {
	let fs: fermenter[] = [];
	for (let f of data) {
	  fs.push({'data': f,
		    'editForm': this.createEditFormGroup(f),
		    'edit': false});
	}
	this.fermenters = fs;
      }
    );
  }

  createEditFormGroup(f: apiFermenter): FormGroup {
    return new FormGroup({
      name: new FormControl(f.name, [Validators.required,
				      Validators.minLength(3)]),
      ftid: new FormControl(f.type.id, [Validators.required]),
    });
  }

  add(model: FormGroup) {
    let ft: apiFermenterType;
    let ftid = model.get('ftid')!.value;
    for (let it of this.fermentertypes) {
      if ( it.id == ftid ) {
	ft = it;
	break;
      }
    }
    let f: apiFermenter = {
      name: model.get('name')!.value,
      type: ft!,
    };
    this.api.addFermenter(this.fermdid!, f)
      .subscribe(
	(data:apiFermenter) => {
	  this.showNew = false;
	  this.updateFermenters();
	}
      );
  }

  del(fid: number) {
    this.api.delFermenter(this.fermdid!, fid)
      .subscribe(
	(data:any) => {
	  this.updateFermenters();
	}
      );
  }

  save(fid: number, model: FormGroup) {
    console.log('savefermenter', fid, model);
    let ft: apiFermenterType;
    let ftid = model.get('ftid')!.value;
    for (let it of this.fermentertypes) {
      if ( it.id == ftid ) {
	ft = it;
	break;
      }
    }
    let f: apiFermenter = {
      name: model.get('name')!.value,
      type: ft!,
    };
    this.api.updateFermenter(this.fermdid!,
			     fid, f)
      .subscribe(
	(data:apiFermenter) => {
	  this.updateFermenters();
	}
      );
  }
}
