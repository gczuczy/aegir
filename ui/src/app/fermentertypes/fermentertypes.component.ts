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

interface fermenterType {
  data: apiFermenterType,
  editForm: FormGroup,
  edit: boolean,
  fermenters: number,
};

@Component({
  selector: 'app-fermentertypes',
  templateUrl: './fermentertypes.component.html',
  styleUrls: ['./fermentertypes.component.css']
})
export class FermentertypesComponent implements OnInit {
  faFileCirclePlus = faFileCirclePlus;

  fermdid: number|null = null;
  public showNew: boolean = false;
  public fermentertypes: fermenterType[] = [];

  public addFermenterTypeForm: FormGroup = new FormGroup({
    name: new FormControl("", [Validators.required,
			       Validators.minLength(3)]),
    capacity: new FormControl(42, [Validators.required,
				   (control: AbstractControl) => Validators.min(10)(control),
				   (control: AbstractControl) => Validators.max(150)(control)]),
    imageurl: new FormControl("", [Validators.required,
				   Validators.pattern("^https?://[A-Za-z0-9./_-]+\.(png|jpg|jpeg)$")]),
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
  }

  updateFermenterTypes() {
    console.log("updateFermenterTypes(), fermdid:", this.fermdid);
    if ( this.fermdid == null ) return;
    this.api.getFermenterTypes(this.fermdid!).subscribe(
      (data:apiFermenterType[]) => {
	let fts: fermenterType[] = [];
	for (let ft of data) {
	  fts.push({'data': ft,
		    'editForm': this.createEditFormGroup(ft),
		    'edit': false,
		    'fermenters': 0});
	}
	this.fermentertypes = fts;
	// counter fermenters on types. If there are fermenters
	// using this type, the type cannot be deleted
	this.api.getFermenters(this.fermdid!).subscribe(
	  (data:apiFermenter[]) => {
	    for (let f of data) {
	      for (let ft of this.fermentertypes) {
		if (f.type.id == ft.data.id) ft.fermenters += 1;
	      }
	    }
	  }
	);
      }
    );
  }

  createEditFormGroup(ft: apiFermenterType): FormGroup {
    return new FormGroup({
      name: new FormControl(ft.name, [Validators.required,
				      Validators.minLength(3)]),
      capacity: new FormControl(ft.capacity, [Validators.required,
					      (control: AbstractControl) => Validators.min(10)(control),
					      (control: AbstractControl) => Validators.max(150)(control)]),
      imageurl: new FormControl(ft.imageurl, [Validators.required,
					      Validators.pattern("^https?://[A-Za-z0-9./_-]+\.(png|jpg|jpeg)$")]),
    });
  }

  add(model: FormGroup) {
    let ft: apiFermenterType = {
      name: model.get('name')!.value,
      capacity: model.get('capacity')!.value,
      imageurl: model.get('imageurl')!.value,
    };
    this.api.addFermenterType(this.fermdid!, ft)
      .subscribe(
	(data:apiFermenterType) => {
	  this.showNew = false;
	  this.updateFermenterTypes();
	}
      );
  }

  del(ftid: number) {
    this.api.delFermenterType(this.fermdid!, ftid)
      .subscribe(
	(data:any) => {
	  this.updateFermenterTypes();
	}
      );
  }

  save(ftid: number, model: FormGroup) {
    console.log('saveft', ftid, model);
    this.api.updateFermenterType(this.fermdid!,
				 ftid, model.value)
      .subscribe(
	(data:apiFermenterType) => {
	  this.updateFermenterTypes();
	}
      );
  }

}
