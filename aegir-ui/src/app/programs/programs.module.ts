import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { RouterModule } from '@angular/router';
import { FormsModule, FormGroup, ReactiveFormsModule } from '@angular/forms';

import { ProgramsComponent } from './programs.component';
import { AddProgramComponent } from './add-program.component';
import { ProgramComponent } from './program.component';

import { PROGRAMS_ROUTES } from './programs.routes';
import { EditProgramComponent } from './edit-program.component';

@NgModule({
    imports: [
	CommonModule,
	FormsModule,
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
	EditProgramComponent
    ]
})
export class ProgramsModule { }
