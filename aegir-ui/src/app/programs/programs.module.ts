import { NgModule } from '@angular/core';
import { BrowserModule } from '@angular/platform-browser';
import { CommonModule } from '@angular/common';
import { RouterModule } from '@angular/router';
import { FormsModule, FormGroup, ReactiveFormsModule } from '@angular/forms';
import { UiSwitchModule } from 'ngx-ui-switch';

import { ProgramsComponent } from './programs.component';
import { AddProgramComponent } from './add-program.component';
import { ProgramComponent } from './program.component';

import { PROGRAMS_ROUTES } from './programs.routes';
import { EditProgramComponent } from './edit-program.component';
import { LoadProgramComponent } from './load-program.component';

@NgModule({
    imports: [
	CommonModule,
	BrowserModule,
	FormsModule,
	UiSwitchModule,
	ReactiveFormsModule,
	RouterModule.forChild(PROGRAMS_ROUTES)
    ],
    exports: [
    ],
    providers: [
    ],
    declarations: [
	ProgramsComponent,
	AddProgramComponent,
	ProgramComponent,
	EditProgramComponent,
	LoadProgramComponent
    ]
})
export class ProgramsModule { }
